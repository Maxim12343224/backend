#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>
#include <boost/log/trivial.hpp>
#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include <boost/program_options.hpp>
#include <filesystem>
#include <boost/system/error_code.hpp>
#include "state_manager.h"

using namespace std::literals;
namespace net = boost::asio;
namespace json = boost::json;
namespace po = boost::program_options;
namespace fs = std::filesystem;

namespace {

    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn) {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        while (--n) {
            workers.emplace_back(fn);
        }
        fn();
    }

    class Ticker : public std::enable_shared_from_this<Ticker> {
    public:
        using Strand = net::strand<net::io_context::executor_type>;
        using Handler = std::function<void(std::chrono::milliseconds delta)>;

        Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
            : strand_(strand)
            , period_(period)
            , handler_(std::move(handler)) {
        }

        void Start() {
            net::dispatch(strand_, [self = shared_from_this()] {
                self->last_tick_ = Clock::now();
                self->ScheduleTick();
            });
        }

    private:
        void ScheduleTick() {
            assert(strand_.running_in_this_thread());
            timer_.expires_after(period_);
            timer_.async_wait(net::bind_executor(
                strand_, 
                [self = shared_from_this()](boost::system::error_code ec) {  
                    self->OnTick(ec);
                }
            ));
        }

        void OnTick(boost::system::error_code ec) {  
            using namespace std::chrono;
            assert(strand_.running_in_this_thread());

            if (!ec) {
                auto this_tick = Clock::now();
                auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
                last_tick_ = this_tick;
                try {
                    handler_(delta);
                } catch (...) {
                }
                ScheduleTick();
            }
        }

        using Clock = std::chrono::steady_clock;

        Strand strand_;
        std::chrono::milliseconds period_;
        net::steady_timer timer_{strand_};
        Handler handler_;
        std::chrono::steady_clock::time_point last_tick_;
    };

}  // namespace

int main(int argc, const char* argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<int>(), "set tick period in milliseconds")
        ("config-file,c", po::value<std::string>()->required(), "set config file path")
        ("www-root,w", po::value<std::string>()->required(), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions")
        ("state-file", po::value<std::string>(), "set state file path")
        ("save-state-period", po::value<int>(), "set save state period in milliseconds");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        
        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }
        
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cout << desc << "\n";
        return EXIT_FAILURE;
    }

    try {
        logger::InitLogging();
        
        const fs::path config_path(vm["config-file"].as<std::string>());
        const fs::path static_path(vm["www-root"].as<std::string>());
        const bool randomize_spawn = vm.count("randomize-spawn-points") > 0;
        const int tick_period = vm.count("tick-period") ? vm["tick-period"].as<int>() : 0;
        const bool is_tick_automatic = tick_period > 0;
        
        fs::path state_file;
        std::chrono::milliseconds save_state_period(0);
        
        if (vm.count("state-file")) {
            state_file = vm["state-file"].as<std::string>();
        }
        
        if (vm.count("save-state-period")) {
            save_state_period = std::chrono::milliseconds(vm["save-state-period"].as<int>());
        }

        model::Game game;
        json_loader::LoadGame(config_path, game);
        game.SetRandomizeSpawnPoints(randomize_spawn);

        StateManager state_manager(game, state_file, save_state_period);
        
        if (!state_file.empty()) {
            try {
                state_manager.LoadState();
            } catch (const std::exception& e) {
                std::cerr << "Failed to load game state: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        auto api_strand = net::make_strand(ioc);

        auto tick_handler = [&game, &state_manager](std::chrono::milliseconds delta) {
            game.Tick(static_cast<double>(delta.count()) / 1000.0);
            state_manager.OnTick(delta);
        };

        if (is_tick_automatic) {
            std::cout << "Starting automatic ticker with period: " << tick_period << " ms" << std::endl;
            auto ticker = std::make_shared<Ticker>(
                api_strand, 
                std::chrono::milliseconds(tick_period),
                tick_handler
            );
            ticker->Start();
        }

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &state_manager](const boost::system::error_code& ec, int) {
            if (!ec) {
                std::cout << "Received shutdown signal, saving state..." << std::endl;
                state_manager.OnShutdown();
                ioc.stop();
            }
        });

        http_handler::RequestHandler base_handler{ game, static_path, is_tick_automatic, config_path };
        http_handler::LoggingRequestHandler handler{ std::move(base_handler) };

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& addr, auto&& send) {
            handler(std::forward<decltype(req)>(req),
                std::forward<decltype(addr)>(addr),
                std::forward<decltype(send)>(send));
        });

        json::value start_data{
            {"port", port},
            {"address", address.to_string()},
            {"static_path", static_path.string()},
            {"config_path", config_path.string()},
            {"randomize_spawn_points", randomize_spawn},
            {"tick_period_ms", tick_period},
            {"state_file", state_file.string()},
            {"save_state_period_ms", save_state_period.count()}
        };
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, start_data)
            << "server started";

        RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });

        json::value exit_data{ {"code", 0} };
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, exit_data)
            << "server exited";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& ex) {
        json::value exit_data{
            {"code", EXIT_FAILURE},
            {"exception", ex.what()}
        };
        BOOST_LOG_TRIVIAL(error) << boost::log::add_value(logger::additional_data, exit_data)
            << "server exited";
        return EXIT_FAILURE;
    }
}
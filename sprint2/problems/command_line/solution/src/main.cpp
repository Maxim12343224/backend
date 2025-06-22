#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include "json_loader.h"
#include "request_handler.h"
#include "ticker.h"
#include "http_server.h"

using namespace std::literals;
namespace net = boost::asio;
namespace po = boost::program_options;
namespace sys = boost::system;
namespace http = boost::beast::http;

namespace {

    // ��������� ��� �������� ���������� ��������� ������
    struct Args {
        std::string config_file;          // ���� � ����������������� �����
        std::string www_root;             // ���� � ����������� ������
        std::optional<int> tick_period;   // ������ ���������� ���� (��)
        bool randomize_spawn_points;      // ��������� ����� ������
    };

    // ������� �������� ���������� ��������� ������
    [[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
        po::options_description desc{ "Game server options" };
        Args args;

        // �������� ����������
        desc.add_options()
            ("help,h", "Show help")
            ("tick-period,t", po::value<int>()->value_name("ms"),
                "Set tick period (milliseconds) for automatic updates")
            ("config-file,c", po::value(&args.config_file)->required()->value_name("file"),
                "Path to game configuration file")
            ("www-root,w", po::value(&args.www_root)->required()->value_name("dir"),
                "Path to static files directory")
            ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points),
                "Spawn dogs at random positions on roads");

        po::variables_map vm;
        try {
            // ������� ��������� ������
            po::store(po::parse_command_line(argc, argv, desc), vm);

            // ����� �������
            if (vm.count("help")) {
                std::cout << desc << "\n";
                return std::nullopt;
            }

            // �������� ������������ ����������
            po::notify(vm);

            // ��������� tick-period
            if (vm.count("tick-period")) {
                args.tick_period = vm["tick-period"].as<int>();
                if (*args.tick_period <= 0) {
                    throw std::runtime_error("Tick period must be positive");
                }
            }

            return args;
        }
        catch (const po::error& e) {
            std::cerr << "Command line error: " << e.what() << "\n";
            std::cerr << desc << "\n";
            return std::nullopt;
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return std::nullopt;
        }
    }

    // ������ ������� �������
    template <typename Fn>
    void RunWorkers(unsigned num_threads, const Fn& fn) {
        num_threads = std::max(1u, num_threads);
        std::vector<std::jthread> workers;
        workers.reserve(num_threads - 1);

        // ������ ������� �������
        while (--num_threads) {
            workers.emplace_back(fn);
        }

        // ������ ������� � �������� ������
        fn();
    }

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        // ������� ���������� ��������� ������
        auto args = ParseCommandLine(argc, argv);
        if (!args) {
            return EXIT_SUCCESS;  // ���������� ����� ��� ������ �������
        }

        // �������� ������� ������
        auto game = json_loader::LoadGame(args->config_file);
        if (args->randomize_spawn_points) {
            game->SetRandomSpawnPoints(true);
        }

        // ����������� ���������� �������
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // ��������� �������� ����������
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, int) {
            if (!ec) {
                ioc.stop();
                std::cout << "Server shutdown initiated..." << std::endl;
            }
            });

        // �������� ����������� ��������
        auto api_strand = net::make_strand(ioc);
        http_handler::RequestHandler handler{ *game, args->www_root, api_strand };

        // ��������� ��������������� ���������� (���� ������ ������)
        if (args->tick_period) {
            auto ticker = std::make_shared<Ticker>(
                api_strand,
                std::chrono::milliseconds(*args->tick_period),
                [&game](std::chrono::milliseconds delta) {
                    game->UpdateState(delta.count());
                }
            );
            ticker->Start();
            std::cout << "Auto-tick enabled (" << *args->tick_period << "ms)" << std::endl;
        }
        else {
            std::cout << "Manual tick mode enabled" << std::endl;
        }

        // ������ HTTP �������
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;

        http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

        std::cout << "Server started at http://" << address << ":" << port << std::endl;
        std::cout << "Config: " << args->config_file << std::endl;
        std::cout << "Static files: " << args->www_root << std::endl;
        std::cout << "Hardware concurrency: " << num_threads << " threads" << std::endl;

        // ������ ������� �������
        RunWorkers(num_threads, [&ioc] {
            ioc.run();
            });
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
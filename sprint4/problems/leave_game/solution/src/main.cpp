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
#include <pqxx/pqxx>
#include <chrono>

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

    // Функция для получения URL базы данных
     std::string GetDatabaseUrl() {
        // 1. Проверяем переменную окружения GAME_DB_URL (основная для тестов)
        const char* env_url = std::getenv("GAME_DB_URL");
        if (env_url && strlen(env_url) > 0) {
            std::string url(env_url);
            
            // Тесты могут использовать postgres:// вместо postgresql://
            if (url.find("postgres://") == 0) {
                url = "postgresql://" + url.substr(11);
                std::cout << "INFO: Fixed URL format: " << url << std::endl;
            }
            
            std::cout << "INFO: Using database URL from GAME_DB_URL: " << url << std::endl;
            return url;
        }
        
        // 2. Проверяем другие переменные тестов
        const char* postgres_host = std::getenv("POSTGRES_HOST");
        const char* postgres_user = std::getenv("POSTGRES_USER");
        const char* postgres_password = std::getenv("POSTGRES_PASSWORD");
        const char* postgres_port = std::getenv("POSTGRES_PORT");
        const char* postgres_db = std::getenv("POSTGRES_DB");
        
        if (postgres_host && postgres_user && postgres_password && postgres_port) {
            std::string db_name = postgres_db ? postgres_db : "game_db";
            std::string url = "postgresql://" + std::string(postgres_user) + ":" +
                             std::string(postgres_password) + "@" +
                             std::string(postgres_host) + ":" +
                             std::string(postgres_port) + "/" + db_name;
            
            std::cout << "INFO: Constructed database URL from test environment: " << url << std::endl;
            return url;
        }
        
        // 3. Для локальной разработки - используем in-memory
        std::cout << "INFO: No PostgreSQL URL found. Using in-memory storage for retired players." << std::endl;
        return "";
    }
    
    // Функция для проверки подключения к PostgreSQL с повторными попытками
    bool WaitForPostgreSQL(const std::string& db_url, int max_attempts = 30) {
        if (db_url.empty()) {
            std::cout << "INFO: No database URL provided, skipping PostgreSQL connection." << std::endl;
            return false;
        }
        
        std::cout << "INFO: Waiting for PostgreSQL to be ready..." << std::endl;
        
        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
            try {
                std::cout << "Attempt " << attempt << "/" << max_attempts << " to connect to PostgreSQL..." << std::endl;
                pqxx::connection conn(db_url);
                
                if (conn.is_open()) {
                    std::cout << "SUCCESS: Connected to PostgreSQL" << std::endl;
                    return true;
                }
            } catch (const std::exception& e) {
                std::cout << "WARNING: PostgreSQL not ready yet: " << e.what() << std::endl;
                
                // Ждем перед следующей попыткой
                if (attempt < max_attempts) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        }
        
        std::cout << "ERROR: Failed to connect to PostgreSQL after " << max_attempts << " attempts" << std::endl;
        return false;
    }

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
        if (!vm.count("config-file") || !vm.count("www-root")) {
            std::cerr << "Error: config-file and www-root are required" << std::endl;
            return EXIT_FAILURE;
        }

        const fs::path config_path(vm["config-file"].as<std::string>());
        const fs::path static_path(vm["www-root"].as<std::string>());
        
        if (!fs::exists(config_path)) {
            std::cerr << "Error: config file not found: " << config_path << std::endl;
            return EXIT_FAILURE;
        }
        
        if (!fs::exists(static_path)) {
            std::cerr << "Error: static path not found: " << static_path << std::endl;
            return EXIT_FAILURE;
        }

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

        logger::InitLogging();
        
        // Получаем URL базы данных
        std::string db_url = GetDatabaseUrl();
        std::cout << "INFO: Final database URL: " << db_url << std::endl;
        
        // Ждем пока PostgreSQL будет готов (особенно важно для тестов)
        bool db_connected = WaitForPostgreSQL(db_url);
        
        if (!db_connected) {
            std::cout << "WARNING: Could not connect to PostgreSQL" << std::endl;
            std::cout << "WARNING: Retired players will be stored in memory only" << std::endl;
            std::cout << "WARNING: Records will not persist between server restarts" << std::endl;
        }

        // Создаем игру
        model::Game game;
        if (db_connected) {
            game.SetDatabaseUrl(db_url);
            std::cout << "INFO: PostgreSQL storage enabled for retired players" << std::endl;
        } else {
            std::cout << "INFO: Using in-memory storage for retired players" << std::endl;
        }
        
        try {
            json_loader::LoadGame(config_path, game);
        } catch (const std::exception& e) {
            std::cerr << "Error loading game config: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        
        game.SetRandomizeSpawnPoints(randomize_spawn);

        StateManager state_manager(game, state_file, save_state_period);
        
        // Загружаем состояние, если указан файл состояния
        if (!state_file.empty()) {
            try {
                if (!state_manager.LoadState()) {
                    std::cout << "INFO: Starting with clean state" << std::endl;
                } else {
                    std::cout << "INFO: Game state loaded successfully" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load game state: " << e.what() << ". Starting with clean state." << std::endl;
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
                state_manager.OnShutdown();
                ioc.stop();
            }
        });

        http_handler::RequestHandler base_handler{ 
            game, static_path, is_tick_automatic, config_path, state_manager 
        };
        http_handler::LoggingRequestHandler handler{ std::move(base_handler) };

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        
        std::cout << "========================================" << std::endl;
        std::cout << "Server starting on " << address << ":" << port << std::endl;
        std::cout << "Static files path: " << static_path << std::endl;
        std::cout << "Config file: " << config_path << std::endl;
        std::cout << "Database: " << (db_connected ? "PostgreSQL" : "In-memory") << std::endl;
        if (db_connected) {
            std::cout << "Database URL: " << db_url << std::endl;
        }
        std::cout << "========================================" << std::endl;
        
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
            {"save_state_period_ms", save_state_period.count()},
            {"database_url", db_url},
            {"database_connected", db_connected}
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
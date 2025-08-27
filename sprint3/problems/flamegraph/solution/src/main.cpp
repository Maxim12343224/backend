#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;

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
}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }

    try {
        // 1. Загружаем карту из файла
        std::cout << "Loading game from: " << argv[1] << std::endl;
        model::Game game = json_loader::LoadGame(argv[1]);

        // Логирование загруженных карт
        std::cout << "Successfully loaded " << game.GetMaps().size() << " maps:" << std::endl;
        for (const auto& map : game.GetMaps()) {
            std::cout << "  - " << *map.GetId() << " (" << map.GetName() << ")" << std::endl;
        }

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Обработка сигналов для корректного завершения
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                std::cout << "Server shutting down..." << std::endl;
                ioc.stop();
            }
            });

        // 4. Создаем обработчик запросов
        http_handler::RequestHandler handler{ game };

        // 5. Запускаем сервер
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

        // Сообщаем о готовности сервера
        std::cout << "Server has started on http://" << address << ":" << port << std::endl;

        // 6. Запускаем обработку запросов
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
            });

    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
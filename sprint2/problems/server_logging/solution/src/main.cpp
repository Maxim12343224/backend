#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>
#include <boost/log/trivial.hpp>
#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"

using namespace std::literals;
namespace net = boost::asio;
namespace json = boost::json;

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
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-files-path>"sv << std::endl;
        return EXIT_FAILURE;
    }

    try {
        logger::InitLogging();
        model::Game game = json_loader::LoadGame(argv[1]);
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int) {
            if (!ec) ioc.stop();
            });

        http_handler::RequestHandler base_handler{ game, argv[2] };
        http_handler::LoggingRequestHandler handler{ std::move(base_handler) };

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& addr, auto&& send) {
            handler(std::forward<decltype(req)>(req),
                std::forward<decltype(addr)>(addr),
                std::forward<decltype(send)>(send));
            });

        // Логирование запуска
        json::value start_data{
            {"port", port},
            {"address", address.to_string()}
        };
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, start_data)
            << "server started";

        RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });

        // Успешное завершение
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
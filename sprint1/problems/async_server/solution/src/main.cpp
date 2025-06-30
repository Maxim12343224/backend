// main.cpp
#include "sdk.h"
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
    namespace net = boost::asio;
    using namespace std::literals;
    namespace sys = boost::system;
    namespace http = boost::beast::http;

    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
    };

    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
        bool keep_alive,
        std::string_view content_type = ContentType::TEXT_HTML) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    StringResponse HandleRequest(StringRequest&& req) {
        const auto method = req.method();
        const auto target = req.target();

        if (method != http::verb::get && method != http::verb::head) {
            auto response = MakeStringResponse(http::status::method_not_allowed, "Invalid method"sv,
                req.version(), req.keep_alive());
            response.set(http::field::allow, "GET, HEAD");
            return response;
        }

        if (target.empty() || target[0] != '/') {
            return MakeStringResponse(http::status::bad_request, "Invalid target"sv,
                req.version(), req.keep_alive());
        }

        std::string_view name = target.substr(1);
        std::string body = "Hello, " + std::string(name);

        auto response = MakeStringResponse(http::status::ok, body, req.version(), req.keep_alive());

        if (method == http::verb::head) {
            response.body().clear();
        }

        return response;
    }

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

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
        });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, { address, port }, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
        });

    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
        });
}
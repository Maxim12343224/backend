#ifdef WIN32
#include <sdkddkver.h>
#endif
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>
#include <string>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

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
    using namespace std::literals;

    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        auto response = MakeStringResponse(http::status::method_not_allowed, "Invalid method"sv, req.version(), req.keep_alive());
        response.set(http::field::allow, "GET, HEAD"sv);
        return response;
    }

    
    std::string target = std::string(req.target());
    if (!target.empty() && target[0] == '/') {
        target = target.substr(1);
    }

    std::string body = "Hello, " + target;
    StringResponse response;

    if (req.method() == http::verb::get) {
        response = MakeStringResponse(http::status::ok, body, req.version(), req.keep_alive());
    }
    else { // HEAD
        response = MakeStringResponse(http::status::ok, ""sv, req.version(), req.keep_alive());
        response.content_length(body.size());
    }

    return response;
}

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (ec) {
        throw std::runtime_error("Failed to read request: "s + ec.message());
    }
    return req;
}

template <typename RequestHandler>
void HandleConnection(tcp::socket socket, RequestHandler&& handle_request) {
    try {
        beast::flat_buffer buffer;
        while (auto request = ReadRequest(socket, buffer)) {
            StringResponse response = handle_request(*std::move(request));
            http::write(socket, response);
            if (response.need_eof()) {
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main() {
    net::io_context ioc;
    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    tcp::acceptor acceptor(ioc, { address, port });
    std::cout << "Server has started..."sv << std::endl;

    while (true) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);

        std::thread t([socket = std::move(socket)]() mutable {
            HandleConnection(std::move(socket), HandleRequest);
            });
        t.detach();
    }
}
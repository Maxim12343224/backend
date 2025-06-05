#pragma once
#include "sdk.h"
#include <iostream>
#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_server {

    namespace net = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    using tcp = net::ip::tcp;

    using RequestHandler = std::function<void(
        http::request<http::string_body>&& req,
        std::function<void(http::response<http::string_body>&&)>&& send)>;

    void ReportError(beast::error_code ec, std::string_view what);

    class Session : public std::enable_shared_from_this<Session> {
    public:
        template <typename Handler>
        Session(tcp::socket&& socket, Handler&& handler)
            : stream_(std::move(socket))
            , request_handler_(std::forward<Handler>(handler)) {
        }

        void Run();

    private:
        void Read();
        void OnRead(beast::error_code ec, std::size_t bytes_read);
        void OnWrite(bool close, beast::error_code ec, std::size_t bytes_written);
        void Close();

        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
        RequestHandler request_handler_;
    };

    template <typename RequestHandler>
    class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
    public:
        template <typename Handler>
        Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& handler)
            : ioc_(ioc)
            , acceptor_(net::make_strand(ioc))
            , request_handler_(std::forward<Handler>(handler)) {
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(net::socket_base::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen(net::socket_base::max_listen_connections);
        }

        void Run() {
            DoAccept();
        }

    private:
        void DoAccept() {
            acceptor_.async_accept(
                net::make_strand(ioc_),
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        void OnAccept(beast::error_code ec, tcp::socket socket) {
            if (ec) {
                return ReportError(ec, "accept");
            }

            std::make_shared<Session>(std::move(socket), request_handler_)->Run();
            DoAccept();
        }

        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        RequestHandler request_handler_;
    };

    template <typename RequestHandler>
    void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
        std::make_shared<Listener<std::decay_t<RequestHandler>>>(
            ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

}  // namespace http_server
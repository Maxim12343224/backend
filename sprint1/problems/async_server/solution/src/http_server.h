#pragma once
#include "sdk.h"
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_server {

    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    namespace beast = boost::beast;
    namespace http = beast::http;

    class SessionBase {
    protected:
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;

        explicit SessionBase(tcp::socket&& socket)
            : stream_(std::move(socket)) {}
    };

    template <typename RequestHandler>
    class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
    public:
        Session(tcp::socket&& socket, RequestHandler handler)
            : SessionBase(std::move(socket)), handler_(std::move(handler)) {}

        void Run() {
            net::dispatch(stream_.get_executor(),
                beast::bind_front_handler(&Session::Read, this->shared_from_this()));
        }

    private:
        RequestHandler handler_;

        void Read() {
            http::async_read(stream_, buffer_, request_,
                beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
        }

        void OnRead(beast::error_code ec, std::size_t) {
            if (ec == http::error::end_of_stream) {
                return Close();
            }
            if (ec) {
                return;
            }

            handler_(std::move(request_),
                [self = this->shared_from_this()](auto&& response) {
                    self->OnWrite(std::forward<decltype(response)>(response));
                });
        }

        void OnWrite(http::response<http::string_body>&& response) {
            auto sp = std::make_shared<http::response<http::string_body>>(std::move(response));

            http::async_write(stream_, *sp,
                [self = this->shared_from_this(), sp](
                    beast::error_code ec, std::size_t) {
                        if (ec) {
                            return;
                        }
                        if (sp->need_eof()) {
                            return self->Close();
                        }
                        self->Read();
                });
        }

        void Close() {
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        }
    };

    template <typename RequestHandler>
    class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
    public:
        Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler handler)
            : ioc_(ioc), acceptor_(ioc), handler_(std::move(handler)) {
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(net::socket_base::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen(net::socket_base::max_listen_connections);
        }

        void Run() {
            DoAccept();
        }

    private:
        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        RequestHandler handler_;

        void DoAccept() {
            acceptor_.async_accept(
                net::make_strand(ioc_),
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        void OnAccept(beast::error_code ec, tcp::socket socket) {
            if (ec) {
                return;
            }
            std::make_shared<Session<RequestHandler>>(
                std::move(socket), handler_)->Run();
            DoAccept();
        }
    };

    template <typename RequestHandler>
    void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
        std::make_shared<Listener<std::decay_t<RequestHandler>>>(
            ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

}  // namespace http_server
// http_server.h
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

    // ќбъ€вим StringResponse здесь, чтобы он был доступен во всех классах
    using StringResponse = http::response<http::string_body>;

    class SessionBase {
    public:
        virtual ~SessionBase() = default;
        virtual void Run() = 0;
    };

    template <typename RequestHandler>
    class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
    public:
        Session(tcp::socket&& socket, const RequestHandler& handler)
            : stream_(std::move(socket))
            , handler_(handler) {
        }

        void Run() override {
            net::dispatch(stream_.get_executor(),
                beast::bind_front_handler(&Session::Read, this->shared_from_this()));
        }

    private:
        void Read() {
            req_ = {};
            stream_.expires_after(std::chrono::seconds(30));
            http::async_read(stream_, buffer_, req_,
                beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
        }

        void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
            if (ec == http::error::end_of_stream) {
                return Close();
            }
            if (ec) {
                return;
            }
            handler_(std::move(req_), [self = this->shared_from_this()](StringResponse&& response) {
                self->Write(std::move(response));
                });
        }

        void Write(StringResponse&& response) {
            bool keep_alive = response.keep_alive();

            http::async_write(stream_, response,
                beast::bind_front_handler(&Session::OnWrite, this->shared_from_this(), keep_alive));
        }

        void OnWrite(bool keep_alive, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
            if (ec) {
                return;
            }
            if (keep_alive) {
                Read();
            }
            else {
                Close();
            }
        }

        void Close() {
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        }

        beast::tcp_stream stream_;
        RequestHandler handler_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
    };

    template <typename RequestHandler>
    class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
    public:
        Listener(net::io_context& ioc, const tcp::endpoint& endpoint, const RequestHandler& handler)
            : ioc_(ioc)
            , acceptor_(net::make_strand(ioc))
            , handler_(handler) {
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(net::socket_base::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen(net::socket_base::max_listen_connections);
        }

        void Run() {
            Accept();
        }

    private:
        void Accept() {
            acceptor_.async_accept(net::make_strand(ioc_),
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        void OnAccept(beast::error_code ec, tcp::socket socket) {
            if (ec) {
                return Accept();
            }
            auto session = std::make_shared<Session<RequestHandler>>(std::move(socket), handler_);
            session->Run();
            Accept();
        }

        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        RequestHandler handler_;
    };

    template <typename RequestHandler>
    void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
        std::make_shared<Listener<std::decay_t<RequestHandler>>>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

}  // namespace http_server
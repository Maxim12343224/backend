#include "http_server.h"
#include <iostream>

namespace http_server {

    void Session::Run() {
        net::dispatch(stream_.get_executor(),
            beast::bind_front_handler(&Session::Read, shared_from_this()));
    }

    void Session::Read() {
        request_ = {};
        stream_.expires_after(std::chrono::seconds(30));
        http::async_read(stream_, buffer_, request_,
            beast::bind_front_handler(&Session::OnRead, shared_from_this()));
    }

    void Session::OnRead(beast::error_code ec, std::size_t bytes_read) {
        if (ec == http::error::end_of_stream) {
            return Close();
        }
        if (ec) {
            return ReportError(ec, "read");
        }

        request_handler_(std::move(request_), [self = shared_from_this()](auto&& response) {
            auto safe_response = std::make_shared<http::response<http::string_body>>(
                std::forward<decltype(response)>(response));
            http::async_write(self->stream_, *safe_response,
                [self, safe_response](beast::error_code ec, std::size_t bytes_written) {
                    self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                });
            });
    }

    void Session::OnWrite(bool close, beast::error_code ec, std::size_t bytes_written) {
        if (ec) {
            return ReportError(ec, "write");
        }
        if (close) {
            return Close();
        }
        Read();
    }

    void Session::Close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

    void ReportError(beast::error_code ec, std::string_view what) {
        std::cerr << what << ": " << ec.message() << std::endl;
    }

}  // namespace http_server
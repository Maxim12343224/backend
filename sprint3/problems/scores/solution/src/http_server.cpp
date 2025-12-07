#include "http_server.h"
#include <boost/json.hpp>

namespace http_server {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

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

        request_handler_(std::move(request_), remote_address_,
            [self = shared_from_this()](auto&& response) {
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
        try {
            stream_.socket().shutdown(tcp::socket::shutdown_send);
        } catch (const std::exception& e) {
            json::value data{
                {"error", e.what()},
                {"where", "close"}
            };
            BOOST_LOG_TRIVIAL(warning) << boost::log::add_value(logger::additional_data, data)
                << "socket shutdown warning";
        }
    }

    void Session::ReportError(beast::error_code ec, std::string_view where) {
        json::value data{
            {"code", ec.value()},
            {"text", ec.message()},
            {"where", std::string(where)}
        };
        BOOST_LOG_TRIVIAL(error) << boost::log::add_value(logger::additional_data, data)
            << "error";
    }

}  // namespace http_server
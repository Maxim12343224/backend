#pragma once

#include "game.h"
#include <filesystem>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/asio/strand.hpp>
#include <optional>

namespace http_handler {

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;
    namespace net = boost::asio;

    using StringResponse = http::response<http::string_body>;
    using StringRequest = http::request<http::string_body>;
    using FileResponse = http::response<http::file_body>;

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game, const fs::path& static_path,
            net::strand<net::io_context::executor_type> strand);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            net::dispatch(
                strand_,
                [this, req = std::move(req), send = std::forward<Send>(send)]() mutable {
                    if (req.method() != http::verb::get && req.method() != http::verb::head &&
                        req.method() != http::verb::post) {
                        return send(MakeErrorResponse(http::status::method_not_allowed,
                            "invalidMethod",
                            "Only GET, HEAD and POST methods are expected", req));
                    }

                    if (req.target().starts_with("/api/")) {
                        return send(HandleApiRequest(std::move(req)));
                    }
                    return send(HandleStaticRequest(std::move(req)));
                });
        }

    private:
        StringResponse HandleApiRequest(StringRequest&& req);
        StringResponse HandleStaticRequest(StringRequest&& req);

        StringResponse HandleJoinGame(StringRequest&& req);
        StringResponse HandleGetPlayers(StringRequest&& req);
        StringResponse HandleGetGameState(StringRequest&& req);
        StringResponse HandlePlayerAction(StringRequest&& req);
        StringResponse HandleTick(StringRequest&& req);

        std::optional<model::Token> ExtractToken(const StringRequest& req) const;

        static StringResponse MakeStringResponse(http::status status, std::string_view body,
            const StringRequest& req,
            std::string_view content_type = "application/json");

        static StringResponse MakeErrorResponse(http::status status, std::string_view code,
            std::string_view message, const StringRequest& req);

        static std::string DecodeUrl(std::string_view url);
        static std::string GetMimeType(std::string_view path);
        static bool IsSubPath(const fs::path& path, const fs::path& base);

        model::Game& game_;
        fs::path static_path_;
        net::strand<net::io_context::executor_type> strand_;
        std::optional<std::chrono::milliseconds> tick_period_;
    };

} // namespace http_handler
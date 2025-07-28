#pragma once
#include "http_server.h"
#include "model.h"
#include <filesystem>
#include <unordered_map>
#include <boost/beast.hpp>
#include <boost/json.hpp>

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;

    using StringResponse = http::response<http::string_body>;
    using StringRequest = http::request<http::string_body>;

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game, const fs::path& static_path)
            : game_{ game }, static_path_{ static_path } {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            StringRequest string_req(std::move(req));

            if (string_req.method() != http::verb::get && string_req.method() != http::verb::head) {
                auto response = MakeErrorResponse(http::status::method_not_allowed,
                    "invalidMethod", "Only GET and HEAD methods are expected", string_req);
                return send(std::move(response));
            }

            if (string_req.target().starts_with("/api/")) {
                auto response = HandleApiRequest(std::move(string_req));
                return send(std::move(response));
            }

            auto response = HandleStaticRequest(std::move(string_req));
            return send(std::move(response));
        }

    private:
        model::Game& game_;
        fs::path static_path_;

        StringResponse HandleApiRequest(StringRequest&& req);
        StringResponse HandleStaticRequest(StringRequest&& req);
        std::string DecodeUrl(beast::string_view url);
        std::string GetMimeType(beast::string_view path);

        StringResponse MakeStringResponse(http::status status, std::string_view body,
            const StringRequest& req, beast::string_view content_type = "application/json");

        StringResponse MakeErrorResponse(http::status status, beast::string_view code,
            beast::string_view message, const StringRequest& req);
    };
}  // namespace http_handler
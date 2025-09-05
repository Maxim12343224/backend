#pragma once
#include <optional>
#include "http_server.h"
#include "model.h"
#include <filesystem>
#include <unordered_map>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>
#include "logger.h"

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;

    using StringResponse = http::response<http::string_body>;
    using StringRequest = http::request<http::string_body>;

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game, const fs::path& static_path, bool is_tick_automatic, const fs::path& config_path)
            : game_{ game }, static_path_{ static_path }, is_tick_automatic_{ is_tick_automatic }, config_path_{ config_path } {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        RequestHandler(RequestHandler&& other) noexcept
            : game_{ other.game_ }, static_path_{ std::move(other.static_path_) },
            is_tick_automatic_{ other.is_tick_automatic_ }, config_path_{ std::move(other.config_path_) } {
        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
            std::string remote_address,
            Send&& send) {
            StringRequest string_req(std::move(req));

            if (string_req.target().starts_with("/api/")) {
                if (string_req.target() == "/api/v1/game/join") {
                    if (string_req.method() == http::verb::post) {
                        auto response = HandleJoinGame(std::move(string_req));
                        return send(std::move(response));
                    }
                    else {
                        auto response = MakeErrorResponse(http::status::method_not_allowed,
                            "invalidMethod", "Only POST method is expected", string_req);
                        response.set(http::field::allow, "POST");
                        return send(std::move(response));
                    }
                }
                else if (string_req.target() == "/api/v1/game/players") {
                    auto response = HandleGetPlayers(std::move(string_req));
                    return send(std::move(response));
                }
                else if (string_req.target() == "/api/v1/game/state") {
                    auto response = HandleGameState(std::move(string_req));
                    return send(std::move(response));
                }
                else if (string_req.target() == "/api/v1/game/player/action") {
                    if (string_req.method() == http::verb::post) {
                        auto response = HandlePlayerAction(std::move(string_req));
                        return send(std::move(response));
                    }
                    else {
                        auto response = MakeErrorResponse(http::status::method_not_allowed,
                            "invalidMethod", "Only POST method is expected", string_req);
                        response.set(http::field::allow, "POST");
                        return send(std::move(response));
                    }
                }
                else if (string_req.target() == "/api/v1/game/tick") {
                    if (string_req.method() == http::verb::post) {
                        auto response = HandleTick(std::move(string_req));
                        return send(std::move(response));
                    }
                    else {
                        auto response = MakeErrorResponse(http::status::method_not_allowed,
                            "invalidMethod", "Only POST method is expected", string_req);
                        response.set(http::field::allow, "POST");
                        return send(std::move(response));
                    }
                }
                else {
                    auto response = HandleApiRequest(std::move(string_req));
                    return send(std::move(response));
                }
            }

            auto response = HandleStaticRequest(std::move(string_req));
            return send(std::move(response));
        }

    private:
        model::Game& game_;
        fs::path static_path_;
        bool is_tick_automatic_;
        fs::path config_path_;

        StringResponse HandleApiRequest(StringRequest&& req);
        StringResponse HandleStaticRequest(StringRequest&& req);
        StringResponse HandleJoinGame(StringRequest&& req);
        StringResponse HandleGetPlayers(StringRequest&& req);
        StringResponse HandleGameState(StringRequest&& req);
        StringResponse HandlePlayerAction(StringRequest&& req);
        StringResponse HandleTick(StringRequest&& req);

        std::optional<std::string> GetTokenFromRequest(const StringRequest& req);

        StringResponse MakeStringResponse(http::status status, std::string_view body,
            const StringRequest& req,
            beast::string_view content_type = "application/json");

        StringResponse MakeErrorResponse(http::status status, beast::string_view code,
            beast::string_view message, const StringRequest& req);

        std::string DecodeUrl(beast::string_view url);
        std::string GetMimeType(beast::string_view path);
        bool IsSubPath(const fs::path& path, const fs::path& base);
    };

    class LoggingRequestHandler {
    public:
        explicit LoggingRequestHandler(RequestHandler&& base_handler)
            : base_handler_(std::move(base_handler)) {
        }

        template <typename Request, typename Send>
        void operator()(Request&& req, std::string remote_address, Send&& send) {
            json::value request_data{
                {"ip", remote_address},
                {"URI", std::string(req.target())},
                {"method", std::string(req.method_string())}
            };
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, request_data)
                << "request received";

            auto start_time = std::chrono::steady_clock::now();

            base_handler_(std::forward<Request>(req), remote_address,
                [this, start_time, remote_address, send = std::forward<Send>(send)]
                (auto&& response) mutable {
                    auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start_time).count();

                    std::string content_type = "null";
                    if (auto it = response.find(http::field::content_type);
                        it != response.end()) {
                        content_type = std::string(it->value());
                    }

                    json::value response_data{
                        {"ip", remote_address},
                        {"response_time", response_time},
                        {"code", static_cast<int>(response.result())},
                        {"content_type", content_type}
                    };
                    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, response_data)
                        << "response sent";

                    send(std::forward<decltype(response)>(response));
                });
        }

    private:
        RequestHandler base_handler_;
    };
}  // namespace http_handler
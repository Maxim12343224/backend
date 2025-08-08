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
        explicit RequestHandler(model::Game& game, const fs::path& static_path)
            : game_{ game }, static_path_{ static_path } {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        RequestHandler(RequestHandler&& other) noexcept
            : game_{ other.game_ }, static_path_{ std::move(other.static_path_) } {
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


                //время
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
            auto start_time = std::chrono::steady_clock::now();
            std::string method = std::string(req.method_string());
            std::string uri = std::string(req.target());

            base_handler_(std::forward<Request>(req), remote_address,
                [this, start_time, remote_address, method, uri, send = std::forward<Send>(send)]
                (auto&& response) mutable {
                    auto end_time = std::chrono::steady_clock::now();
                    auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time - start_time).count();

                    // Добавляем логирование "request handled"
                    json::value log_data{
                        {"request", uri},
                        {"method", method},
                        {"response_time", static_cast<double>(response_time) / 1000.0} // в секундах
                    };

                    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, log_data)
                        << "request handled";  // <-- Сообщение, которое ожидает тест

                    send(std::forward<decltype(response)>(response));
                });
        }

    private:
        RequestHandler base_handler_;
    };
}  // namespace http_handler
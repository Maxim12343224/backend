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
#include "state_manager.h"
#include "retired_players_repository.h"  // Добавьте этот include

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;

    using StringResponse = http::response<http::string_body>;
    using StringRequest = http::request<http::string_body>;

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game, const fs::path& static_path,
            bool is_tick_automatic, const fs::path& config_path,
            StateManager& state_manager,
            std::shared_ptr<model::RetiredPlayersRepository> retired_repo = nullptr)  // Добавьте этот параметр
            : game_{ game }, static_path_{ static_path },
            is_tick_automatic_{ is_tick_automatic }, config_path_{ config_path },
            state_manager_{ state_manager }, retired_repo_{ std::move(retired_repo) } {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        RequestHandler(RequestHandler&& other) noexcept
            : game_{ other.game_ }, static_path_{ std::move(other.static_path_) },
            is_tick_automatic_{ other.is_tick_automatic_ }, config_path_{ std::move(other.config_path_) },
            state_manager_{ other.state_manager_ }, retired_repo_{ std::move(other.retired_repo_) } {
        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
            std::string remote_address,
            Send&& send);

    private:
        model::Game& game_;
        fs::path static_path_;
        bool is_tick_automatic_;
        fs::path config_path_;
        StateManager& state_manager_;
        std::shared_ptr<model::RetiredPlayersRepository> retired_repo_;  // Добавьте это поле

        StringResponse HandleApiRequest(StringRequest&& req);
        StringResponse HandleStaticRequest(StringRequest&& req);
        StringResponse HandleJoinGame(StringRequest&& req);
        StringResponse HandleGetPlayers(StringRequest&& req);
        StringResponse HandleGameState(StringRequest&& req);
        StringResponse HandlePlayerAction(StringRequest&& req);
        StringResponse HandleTick(StringRequest&& req);
        StringResponse HandleGetRecords(StringRequest&& req);  // Добавьте этот метод

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

}

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
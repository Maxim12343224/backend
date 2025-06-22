#include "request_handler.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <fstream>

namespace http_handler {

    using namespace std::literals;
    namespace fs = std::filesystem;

    RequestHandler::RequestHandler(model::Game& game, const fs::path& static_path,
        net::strand<net::io_context::executor_type> strand)
        : game_(game)
        , static_path_(static_path)
        , strand_(strand) {
    }

    StringResponse RequestHandler::HandleApiRequest(StringRequest&& req) {
        if (req.target() == "/api/v1/game/join" && req.method() == http::verb::post) {
            return HandleJoinGame(std::move(req));
        }
        if (req.target() == "/api/v1/game/players" &&
            (req.method() == http::verb::get || req.method() == http::verb::head)) {
            return HandleGetPlayers(std::move(req));
        }
        if (req.target() == "/api/v1/game/state" &&
            (req.method() == http::verb::get || req.method() == http::verb::head)) {
            return HandleGetGameState(std::move(req));
        }
        if (req.target() == "/api/v1/game/player/action" && req.method() == http::verb::post) {
            return HandlePlayerAction(std::move(req));
        }
        if (req.target() == "/api/v1/game/tick" && req.method() == http::verb::post) {
            return HandleTick(std::move(req));
        }
        return MakeErrorResponse(http::status::bad_request,
            "badRequest",
            "Bad request", req);
    }

    StringResponse RequestHandler::HandleJoinGame(StringRequest&& req) {
        if (req.find(http::field::content_type) == req.end() ||
            req[http::field::content_type] != "application/json") {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument",
                "Invalid content type", req);
        }

        try {
            auto json_body = json::parse(req.body());
            auto user_name = json_body.at("userName").as_string();
            auto map_id = json_body.at("mapId").as_string();

            if (user_name.empty()) {
                return MakeErrorResponse(http::status::bad_request,
                    "invalidArgument",
                    "Invalid name", req);
            }

            if (auto player = game_.JoinGame(std::string(user_name), model::Map::Id{ std::string(map_id) })) {
                json::object response;
                response["authToken"] = player->GetToken().GetUnderlying();
                response["playerId"] = player->GetId();

                auto resp = MakeStringResponse(http::status::ok,
                    json::serialize(json::value(response)), req, "application/json");
                resp.set(http::field::cache_control, "no-cache");
                return resp;
            }

            return MakeErrorResponse(http::status::not_found,
                "mapNotFound",
                "Map not found", req);

        }
        catch (const std::exception& e) {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument",
                "Join game request parse error", req);
        }
    }

    StringResponse RequestHandler::HandleGetPlayers(StringRequest&& req) {
        if (auto token = ExtractToken(req)) {
            if (auto player = game_.FindPlayerByToken(*token)) {
                json::object players;
                for (const auto& p : game_.GetPlayers()) {
                    if (p->GetDog().GetMap()->GetId() == player->GetDog().GetMap()->GetId()) {
                        json::object player_info;
                        player_info["name"] = p->GetName();
                        players[std::to_string(p->GetId())] = player_info;
                    }
                }
                auto resp = MakeStringResponse(http::status::ok,
                    json::serialize(players), req, "application/json");
                resp.set(http::field::cache_control, "no-cache");
                return resp;
            }
            return MakeErrorResponse(http::status::unauthorized,
                "unknownToken",
                "Player token has not been found", req);
        }
        return MakeErrorResponse(http::status::unauthorized,
            "invalidToken",
            "Authorization header is missing", req);
    }

    StringResponse RequestHandler::HandleGetGameState(StringRequest&& req) {
        if (auto token = ExtractToken(req)) {
            if (auto player = game_.FindPlayerByToken(*token)) {
                json::object players;
                for (const auto& p : game_.GetPlayers()) {
                    if (p->GetDog().GetMap()->GetId() == player->GetDog().GetMap()->GetId()) {
                        auto pos = p->GetDog().GetPosition();
                        auto speed = p->GetDog().GetSpeed();

                        json::array pos_arr = { pos[0], pos[1] };
                        json::array speed_arr = { speed[0], speed[1] };

                        json::object player_info;
                        player_info["pos"] = pos_arr;
                        player_info["speed"] = speed_arr;
                        player_info["dir"] = std::string(1, static_cast<char>(p->GetDog().GetDirection()));

                        players[std::to_string(p->GetId())] = player_info;
                    }
                }

                json::object result;
                result["players"] = players;

                auto resp = MakeStringResponse(http::status::ok,
                    json::serialize(result), req);
                resp.set(http::field::cache_control, "no-cache");
                return resp;
            }
            return MakeErrorResponse(http::status::unauthorized,
                "unknownToken", "Player token has not been found", req);
        }
        return MakeErrorResponse(http::status::unauthorized,
            "invalidToken", "Authorization header is missing", req);
    }

    StringResponse RequestHandler::HandlePlayerAction(StringRequest&& req) {
        if (req.method() != http::verb::post) {
            auto response = MakeErrorResponse(http::status::method_not_allowed,
                "invalidMethod", "Only POST method is allowed", req);
            response.set(http::field::allow, "POST");
            return response;
        }

        if (req.find(http::field::content_type) == req.end() ||
            req[http::field::content_type] != "application/json") {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument",
                "Invalid content type", req);
        }

        if (auto token = ExtractToken(req)) {
            if (auto player = game_.FindPlayerByToken(*token)) {
                try {
                    auto json_body = json::parse(req.body());
                    auto move = json_body.at("move").as_string();

                    auto& dog = player->GetDog();
                    double speed = dog.GetMap()->GetDogSpeed();

                    if (move == "L") {
                        dog.SetDirection(model::Dog::Direction::WEST);
                        dog.SetSpeed(-speed, 0);
                    }
                    else if (move == "R") {
                        dog.SetDirection(model::Dog::Direction::EAST);
                        dog.SetSpeed(speed, 0);
                    }
                    else if (move == "U") {
                        dog.SetDirection(model::Dog::Direction::NORTH);
                        dog.SetSpeed(0, -speed);
                    }
                    else if (move == "D") {
                        dog.SetDirection(model::Dog::Direction::SOUTH);
                        dog.SetSpeed(0, speed);
                    }
                    else if (move == "") {
                        dog.SetSpeed(0, 0);
                    }
                    else {
                        return MakeErrorResponse(http::status::bad_request,
                            "invalidArgument", "Invalid move value", req);
                    }

                    return MakeStringResponse(http::status::ok, "{}", req, "application/json");
                }
                catch (const std::exception& e) {
                    return MakeErrorResponse(http::status::bad_request,
                        "invalidArgument", "Failed to parse action", req);
                }
            }
            return MakeErrorResponse(http::status::unauthorized,
                "unknownToken", "Player token has not been found", req);
        }
        return MakeErrorResponse(http::status::unauthorized,
            "invalidToken", "Authorization header is missing", req);
    }

    StringResponse RequestHandler::HandleTick(StringRequest&& req) {
        if (tick_period_.has_value()) {
            return MakeErrorResponse(http::status::bad_request,
                "badRequest",
                "Manual tick is disabled in auto-tick mode", req);
        }

        if (req.method() != http::verb::post) {
            auto response = MakeErrorResponse(http::status::method_not_allowed,
                "invalidMethod", "Only POST method is allowed", req);
            response.set(http::field::allow, "POST");
            return response;
        }

        if (req.find(http::field::content_type) == req.end() ||
            req[http::field::content_type] != "application/json") {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument",
                "Invalid content type", req);
        }

        try {
            auto json_body = json::parse(req.body());
            auto time_delta = json_body.at("timeDelta").as_int64();

            if (time_delta <= 0) {
                return MakeErrorResponse(http::status::bad_request,
                    "invalidArgument", "timeDelta must be positive", req);
            }

            game_.UpdateState(static_cast<int>(time_delta));

            auto resp = MakeStringResponse(http::status::ok, "{}", req, "application/json");
            resp.set(http::field::cache_control, "no-cache");
            return resp;
        }
        catch (const std::exception& e) {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument", "Failed to parse tick request JSON", req);
        }
    }

    std::optional<model::Token> RequestHandler::ExtractToken(const StringRequest& req) const {
        if (auto it = req.find(http::field::authorization); it != req.end()) {
            auto auth = it->value();
            if (auth.starts_with("Bearer ")) {
                return model::Token{ std::string(auth.substr(7)) };
            }
        }
        return std::nullopt;
    }

    StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body,
        const StringRequest& req, std::string_view content_type) {
        StringResponse response(status, req.version());
        response.set(http::field::content_type,
            beast::string_view(content_type.data(), content_type.size()));
        response.body() = body;
        response.prepare_payload();
        response.keep_alive(req.keep_alive());
        return response;
    }

    StringResponse RequestHandler::MakeErrorResponse(http::status status, std::string_view code,
        std::string_view message, const StringRequest& req) {
        json::object json_res;
        json_res["code"] = std::string(code);
        json_res["message"] = std::string(message);

        auto response = MakeStringResponse(status, json::serialize(json_res), req);
        response.set(http::field::cache_control, "no-cache");

        if (status == http::status::method_not_allowed) {
            if (req.target() == "/api/v1/game/join") {
                response.set(http::field::allow, "POST");
            }
            else if (req.target() == "/api/v1/game/players" || req.target() == "/api/v1/game/state") {
                response.set(http::field::allow, "GET, HEAD");
            }
            else if (req.target() == "/api/v1/game/player/action" || req.target() == "/api/v1/game/tick") {
                response.set(http::field::allow, "POST");
            }
        }
        return response;
    }

    std::string RequestHandler::DecodeUrl(std::string_view url) {
        std::ostringstream decoded;
        for (size_t i = 0; i < url.size(); ++i) {
            if (url[i] == '%') {
                if (i + 2 >= url.size()) {
                    throw std::runtime_error("Invalid URL encoding");
                }
                int hex;
                std::istringstream hex_stream(std::string(url.substr(i + 1, 2)));
                if (!(hex_stream >> std::hex >> hex)) {
                    throw std::runtime_error("Invalid URL encoding");
                }
                decoded << static_cast<char>(hex);
                i += 2;
            }
            else if (url[i] == '+') {
                decoded << ' ';
            }
            else {
                decoded << url[i];
            }
        }
        return decoded.str();
    }

    std::string RequestHandler::GetMimeType(std::string_view path) {
        static const std::unordered_map<std::string_view, std::string_view> mime_types = {
            {".htm", "text/html"},
            {".html", "text/html"},
            {".css", "text/css"},
            {".txt", "text/plain"},
            {".js", "text/javascript"},
            {".json", "application/json"},
            {".xml", "application/xml"},
            {".png", "image/png"},
            {".jpg", "image/jpeg"},
            {".jpe", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".gif", "image/gif"},
            {".bmp", "image/bmp"},
            {".ico", "image/vnd.microsoft.icon"},
            {".tiff", "image/tiff"},
            {".tif", "image/tiff"},
            {".svg", "image/svg+xml"},
            {".svgz", "image/svg+xml"},
            {".mp3", "audio/mpeg"}
        };

        auto pos = path.rfind('.');
        if (pos == std::string_view::npos) {
            return "application/octet-stream";
        }

        std::string ext(path.substr(pos));
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return std::tolower(c);
            });

        auto it = mime_types.find(ext);
        if (it == mime_types.end()) {
            return "application/octet-stream";
        }
        return std::string(it->second);
    }

    bool RequestHandler::IsSubPath(const fs::path& path, const fs::path& base) {
        auto relative = fs::relative(path, base);
        return !relative.empty() && relative.native()[0] != '.';
    }

    StringResponse RequestHandler::HandleStaticRequest(StringRequest&& req) {
        try {
            auto target = req.target();
            std::string_view target_sv(target.data(), target.size());
            auto path = DecodeUrl(target_sv);

            if (path.empty() || path[0] != '/') {
                throw std::runtime_error("Invalid path");
            }
            path = path.substr(1);

            auto full_path = fs::weakly_canonical(static_path_ / path);

            if (!IsSubPath(full_path, static_path_)) {
                throw std::runtime_error("Invalid path");
            }

            if (fs::is_directory(full_path)) {
                full_path /= "index.html";
            }

            if (!fs::exists(full_path)) {
                throw std::runtime_error("File not found");
            }

            std::ifstream file(full_path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Failed to open file");
            }

            std::string content((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

            auto resp = MakeStringResponse(http::status::ok, content, req, GetMimeType(full_path.string()));
            resp.set(http::field::cache_control, "no-cache");
            return resp;
        }
        catch (const std::exception& e) {
            if (std::string_view(e.what()) == "File not found") {
                return MakeStringResponse(http::status::not_found,
                    "File not found", req, "text/plain");
            }
            return MakeStringResponse(http::status::bad_request,
                e.what(), req, "text/plain");
        }
    }
} // namespace http_handler
#include "request_handler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <stdexcept>
#include <iterator>
#include <filesystem>
#include <optional>
#include <random>
#include <chrono>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

namespace {
    json::value SerializeRoad(const model::Road& road) {
        if (road.IsHorizontal()) {
            return {
                {"x0", road.GetStart().x},
                {"y0", road.GetStart().y},
                {"x1", road.GetEnd().x}
            };
        }
        return {
            {"x0", road.GetStart().x},
            {"y0", road.GetStart().y},
            {"y1", road.GetEnd().y}
        };
    }

    json::value SerializeBuilding(const model::Building& building) {
        const auto& bounds = building.GetBounds();
        return {
            {"x", bounds.position.x},
            {"y", bounds.position.y},
            {"w", bounds.size.width},
            {"h", bounds.size.height}
        };
    }

    json::value SerializeOffice(const model::Office& office) {
        return {
            {"id", *office.GetId()},
            {"x", office.GetPosition().x},
            {"y", office.GetPosition().y},
            {"offsetX", office.GetOffset().dx},
            {"offsetY", office.GetOffset().dy}
        };
    }
} // namespace

StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body,
                                                const StringRequest& req, 
                                                beast::string_view content_type) {
    StringResponse response(status, req.version());
    response.set(http::field::content_type, std::string(content_type));
    response.set(http::field::cache_control, "no-cache");
    response.content_length(body.size());
    response.body() = body;
    response.prepare_payload();
    response.keep_alive(req.keep_alive());
    return response;
}

StringResponse RequestHandler::MakeErrorResponse(http::status status, beast::string_view code,
                                               beast::string_view message, const StringRequest& req) {
    json::value json_res{
        {"code", std::string(code)},
        {"message", std::string(message)}
    };
    auto response = MakeStringResponse(status, json::serialize(json_res), req);
    response.set(http::field::cache_control, "no-cache");
    return response;
}

StringResponse RequestHandler::HandleJoinGame(StringRequest&& req) {
    if (req.method() != http::verb::post) {
        auto response = MakeErrorResponse(http::status::method_not_allowed,
                                        "invalidMethod",
                                        "Only POST method is expected", req);
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
        if (!json_body.is_object()) {
            throw std::runtime_error("Request body must be JSON object");
        }

        auto& obj = json_body.as_object();
        if (!obj.contains("userName") || !obj.contains("mapId")) {
            throw std::runtime_error("Missing required fields");
        }

        auto user_name = obj["userName"].as_string();
        auto map_id = obj["mapId"].as_string();

        if (user_name.empty()) {
            return MakeErrorResponse(http::status::bad_request,
                                   "invalidArgument",
                                   "Invalid name", req);
        }

        auto player = game_.JoinGame(model::Map::Id{std::string(map_id)}, std::string(user_name));
        if (!player) {
            return MakeErrorResponse(http::status::not_found,
                                   "mapNotFound",
                                   "Map not found", req);
        }

        json::value response_json{
            {"authToken", *player->GetToken()},
            {"playerId", *player->GetId()}
        };

        return MakeStringResponse(http::status::ok, json::serialize(response_json), req);
    } catch (const std::exception& e) {
        return MakeErrorResponse(http::status::bad_request,
                               "invalidArgument",
                               "Join game request parse error", req);
    }
}

StringResponse RequestHandler::HandleGetPlayers(StringRequest&& req) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        auto response = MakeErrorResponse(http::status::method_not_allowed,
                                        "invalidMethod",
                                        "Invalid method", req);
        response.set(http::field::allow, "GET, HEAD");
        return response;
    }

    auto token = GetTokenFromRequest(req);
    if (!token) {
        return MakeErrorResponse(http::status::unauthorized,
                               "invalidToken",
                               "Authorization header is missing", req);
    }

    auto player = game_.FindPlayerByToken(model::Player::Token{*token});
    if (!player) {
        return MakeErrorResponse(http::status::unauthorized,
                               "unknownToken",
                               "Player token has not been found", req);
    }

    auto session = player->GetSession();
    json::value players_json = json::object();
    for (const auto& p : session->GetPlayers()) {
        players_json.as_object()[std::to_string(*p->GetId())] = {
            {"name", p->GetDog().GetName()}
        };
    }

    return MakeStringResponse(http::status::ok, json::serialize(players_json), req);
}

StringResponse RequestHandler::HandleGameState(StringRequest&& req) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        auto response = MakeErrorResponse(http::status::method_not_allowed,
                                        "invalidMethod",
                                        "Invalid method", req);
        response.set(http::field::allow, "GET, HEAD");
        return response;
    }

    auto token = GetTokenFromRequest(req);
    if (!token) {
        return MakeErrorResponse(http::status::unauthorized,
                               "invalidToken",
                               "Authorization header is required", req);
    }

    auto player = game_.FindPlayerByToken(model::Player::Token{*token});
    if (!player) {
        return MakeErrorResponse(http::status::unauthorized,
                               "unknownToken",
                               "Player token has not been found", req);
    }

    auto session = player->GetSession();
    json::value players_json = json::object();
    
    for (const auto& p : session->GetPlayers()) {
        const auto& dog = p->GetDog();
        players_json.as_object()[std::to_string(*p->GetId())] = {
            {"pos", json::array({dog.GetPosition().x, dog.GetPosition().y})},
            {"speed", json::array({dog.GetSpeed().x, dog.GetSpeed().y})},
            {"dir", model::DirectionToString(dog.GetDirection())}
        };
    }

    return MakeStringResponse(
        http::status::ok,
        json::serialize(json::value{{"players", players_json}}),
        req
    );
}

StringResponse RequestHandler::HandlePlayerAction(StringRequest&& req) {
    if (req.method() != http::verb::post) {
        auto response = MakeErrorResponse(http::status::method_not_allowed,
                                        "invalidMethod",
                                        "Only POST method is expected", req);
        response.set(http::field::allow, "POST");
        return response;
    }

    if (req.find(http::field::content_type) == req.end() || 
        req[http::field::content_type] != "application/json") {
        return MakeErrorResponse(http::status::bad_request,
                               "invalidArgument",
                               "Invalid content type", req);
    }

    auto token = GetTokenFromRequest(req);
    if (!token) {
        return MakeErrorResponse(http::status::unauthorized,
                               "invalidToken",
                               "Authorization header is required", req);
    }

    auto player = game_.FindPlayerByToken(model::Player::Token{*token});
    if (!player) {
        return MakeErrorResponse(http::status::unauthorized,
                               "unknownToken",
                               "Player token has not been found", req);
    }

    try {
        auto json_body = json::parse(req.body());
        if (!json_body.is_object() || !json_body.as_object().contains("move")) {
            return MakeErrorResponse(http::status::bad_request,
                                  "invalidArgument",
                                  "Failed to parse action", req);
        }

        auto move = json_body.as_object()["move"].as_string();
        std::string move_str = std::string(move);
        if (move_str != "L" && move_str != "R" && move_str != "U" && move_str != "D" && move_str != "") {
            return MakeErrorResponse(http::status::bad_request,
                                  "invalidArgument",
                                  "Invalid move value", req);
        }

        game_.SetPlayerAction(model::Player::Token{*token}, move_str);

        return MakeStringResponse(http::status::ok, "{}", req);
    } catch (const std::exception& e) {
        return MakeErrorResponse(http::status::bad_request,
                               "invalidArgument",
                               "Failed to parse action", req);
    }
}

StringResponse RequestHandler::HandleTick(StringRequest&& req) {
    if (req.method() != http::verb::post) {
        auto response = MakeErrorResponse(http::status::method_not_allowed,
            "invalidMethod", "Only POST method is expected", req);
        response.set(http::field::allow, "POST");
        return response;
    }

    if (req.find(http::field::content_type) == req.end() || 
        req[http::field::content_type] != "application/json") {
        return MakeErrorResponse(http::status::bad_request,
            "invalidArgument", "Invalid content type", req);
    }

    try {
        auto json_body = json::parse(req.body());
        if (!json_body.is_object() || !json_body.as_object().contains("timeDelta")) {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument", "Failed to parse tick request JSON", req);
        }

        auto time_delta = json_body.as_object()["timeDelta"];
        if (!time_delta.is_int64()) {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument", "timeDelta must be integer", req);
        }

        auto delta = time_delta.as_int64();
        if (delta < 0) {
            return MakeErrorResponse(http::status::bad_request,
                "invalidArgument", "timeDelta must be non-negative", req);
        }

        game_.Tick(static_cast<double>(delta) / 1000.0);

        return MakeStringResponse(http::status::ok, "{}", req);
    } catch (const std::exception& e) {
        return MakeErrorResponse(http::status::bad_request,
            "invalidArgument", "Failed to parse tick request JSON", req);
    }
}

std::optional<std::string> RequestHandler::GetTokenFromRequest(const StringRequest& req) {
    if (auto it = req.find(http::field::authorization); it != req.end()) {
        auto auth_header = it->value();
        if (auth_header.starts_with("Bearer ")) {
            std::string token = std::string(auth_header.substr(7));
            if (token.length() != 32) {
                return std::nullopt;
            }
            return token;
        }
    }
    return std::nullopt;
}

StringResponse RequestHandler::HandleApiRequest(StringRequest&& req) {
    if (req.target() == "/api/v1/maps") {
        json::array maps_json;
        for (const auto& map : game_.GetMaps()) {
            maps_json.push_back({
                {"id", *map.GetId()},
                {"name", map.GetName()}
            });
        }
        return MakeStringResponse(http::status::ok,
                                json::serialize(maps_json), req);
    }

    if (req.target().starts_with("/api/v1/maps/")) {
        std::string target = req.target().to_string();
        size_t last_slash_pos = target.find_last_of('/');
        if (last_slash_pos == std::string::npos || last_slash_pos == target.size() - 1) {
            return MakeErrorResponse(http::status::bad_request,
                                   "badRequest",
                                   "Invalid map ID format", req);
        }

        std::string map_id = target.substr(last_slash_pos + 1);
        size_t question_pos = map_id.find('?');
        if (question_pos != std::string::npos) {
            map_id = map_id.substr(0, question_pos);
        }

        if (!map_id.empty() && map_id.back() == '/') {
            map_id.pop_back();
        }

        if (const auto* map = game_.FindMap(model::Map::Id{map_id})) {
            json::value map_json{
                {"id", *map->GetId()},
                {"name", map->GetName()},
                {"roads", json::array()},
                {"buildings", json::array()},
                {"offices", json::array()}
            };

            for (const auto& road : map->GetRoads()) {
                map_json.as_object()["roads"].as_array().push_back(SerializeRoad(road));
            }

            for (const auto& building : map->GetBuildings()) {
                map_json.as_object()["buildings"].as_array().push_back(SerializeBuilding(building));
            }

            for (const auto& office : map->GetOffices()) {
                map_json.as_object()["offices"].as_array().push_back(SerializeOffice(office));
            }

            return MakeStringResponse(http::status::ok,
                                    json::serialize(map_json), req);
        }

        return MakeErrorResponse(http::status::not_found,
                               "mapNotFound",
                               "Map not found", req);
    }

    return MakeErrorResponse(http::status::bad_request,
                           "badRequest",
                           "Bad request", req);
}

StringResponse RequestHandler::HandleStaticRequest(StringRequest&& req) {
    try {
        auto path = DecodeUrl(req.target());

        if (path.empty() || path[0] != '/') {
            return MakeStringResponse(http::status::bad_request,
                                    "Invalid path", req, "text/plain");
        }
        path = path.substr(1);

        auto full_path = fs::weakly_canonical(static_path_ / path);

        if (!IsSubPath(full_path, static_path_)) {
            return MakeStringResponse(http::status::bad_request,
                                    "Invalid path: attempted directory traversal",
                                    req, "text/plain");
        }

        if (fs::is_directory(full_path)) {
            full_path /= "index.html";
        }

        if (!fs::exists(full_path)) {
            return MakeStringResponse(http::status::not_found,
                                    "File not found", req, "text/plain");
        }

        std::ifstream file(full_path, std::ios::binary);
        if (!file) {
            return MakeStringResponse(http::status::internal_server_error,
                                    "Failed to open file", req, "text/plain");
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

        return MakeStringResponse(http::status::ok, content, req, GetMimeType(full_path.string()));
    }
    catch (const std::exception& e) {
        return MakeStringResponse(http::status::internal_server_error,
                                "Internal server error", req, "text/plain");
    }
}

std::string RequestHandler::DecodeUrl(beast::string_view url) {
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

std::string RequestHandler::GetMimeType(beast::string_view path) {
    static const std::unordered_map<std::string, std::string> mime_types = {
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
        {".mp3", "audio/mpeg"},
        {".wasm", "application/wasm"}
    };

    std::string path_str(path);
    auto pos = path_str.rfind('.');
    if (pos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = path_str.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    auto it = mime_types.find(ext);
    if (it == mime_types.end()) {
        return "application/octet-stream";
    }
    return it->second;
}

bool RequestHandler::IsSubPath(const fs::path& path, const fs::path& base) {
    try {
        const auto norm_path = fs::weakly_canonical(path);
        const auto norm_base = fs::weakly_canonical(base);

        auto base_it = norm_base.begin();
        auto path_it = norm_path.begin();

        for (; base_it != norm_base.end(); ++base_it, ++path_it) {
            if (path_it == norm_path.end() || *path_it != *base_it) {
                return false;
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}
}  // namespace http_handler
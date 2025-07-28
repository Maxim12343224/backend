#include "request_handler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <stdexcept>
#include <iterator>
#include <iostream>
#include <system_error>

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
        const StringRequest& req, beast::string_view content_type) {
        StringResponse response(status, req.version());
        response.set(http::field::content_type, std::string(content_type));
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(req.keep_alive());
        return response;
    }

    StringResponse RequestHandler::MakeErrorResponse(http::status status, beast::string_view code,
        beast::string_view message, const StringRequest& req) {
        json::value json_res{
            {"code", std::string(code)},
            {"message", std::string(message)}
        };
        return MakeStringResponse(status, json::serialize(json_res), req, "application/json");
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
            return MakeStringResponse(http::status::ok, json::serialize(maps_json), req);
        }

        if (req.target().starts_with("/api/v1/maps/")) {
            std::string target = req.target().to_string();
            size_t last_slash_pos = target.find_last_of('/');
            if (last_slash_pos == std::string::npos || last_slash_pos == target.size() - 1) {
                return MakeErrorResponse(http::status::bad_request,
                    "badRequest", "Invalid map ID format", req);
            }

            std::string map_id = target.substr(last_slash_pos + 1);
            size_t question_pos = map_id.find('?');
            if (question_pos != std::string::npos) {
                map_id = map_id.substr(0, question_pos);
            }

            if (!map_id.empty() && map_id.back() == '/') {
                map_id.pop_back();
            }

            if (const auto* map = game_.FindMap(model::Map::Id{ map_id })) {
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

                return MakeStringResponse(http::status::ok, json::serialize(map_json), req);
            }

            return MakeErrorResponse(http::status::not_found,
                "mapNotFound", "Map not found", req);
        }

        return MakeErrorResponse(http::status::bad_request,
            "badRequest", "Bad request", req);
    }

    StringResponse RequestHandler::HandleStaticRequest(StringRequest&& req) {
        try {
            beast::string_view target = req.target();

            // Handle root request
            if (target == "/" || target == "/index.html") {
                target = "/index.html";
            }

            // Decode URL to path
            std::string path = DecodeUrl(target);

            // Remove leading slash
            while (!path.empty() && path[0] == '/') {
                path = path.substr(1);
            }

            // Handle directory requests
            if (path.empty()) {
                path = "index.html";
            }
            else if (path.back() == '/') {
                path += "index.html";
            }

            // Build full path
            fs::path file_path = static_path_ / path;

            // Normalize path separators
            file_path = file_path.lexically_normal();

            // Simplify security check - only prevent obvious directory traversal
            std::string file_str = file_path.string();
            if (file_str.find("..") != std::string::npos) {
                return MakeStringResponse(http::status::bad_request,
                    "Invalid path", req, "text/plain");
            }

            // Try to open file directly without complex checks
            std::ifstream file(file_path, std::ios::binary);
            if (!file) {
                // Try adding index.html for directories
                if (path.back() != '/') {
                    file_path = static_path_ / path / "index.html";
                    file.open(file_path, std::ios::binary);
                }

                if (!file) {
                    return MakeStringResponse(http::status::not_found,
                        "File not found", req, "text/plain");
                }
            }

            // Read file content
            std::string content((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

            // Get MIME type
            std::string mime_type = GetMimeType(file_path.string());

            return MakeStringResponse(http::status::ok, content, req, mime_type);
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
                    return "";
                }
                int hex;
                std::istringstream hex_stream(std::string(url.substr(i + 1, 2)));
                if (!(hex_stream >> std::hex >> hex)) {
                    return "";
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
            {".mp3", "audio/mpeg"}
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
}  // namespace http_handler
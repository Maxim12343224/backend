#include "request_handler.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
        const auto text_response = [&req](http::status status, std::string_view text) {
            StringResponse response(status, req.version());
            response.set(http::field::content_type, "application/json");
            response.body() = text;
            response.prepare_payload();
            response.keep_alive(req.keep_alive());
            return response;
            };

        const auto error_response = [&text_response](http::status status, std::string_view code, std::string_view message) {
            json::value json_res{
                {"code", code},
                {"message", message}
            };
            return text_response(status, json::serialize(json_res));
            };

        if (req.method() != http::verb::get) {
            return error_response(http::status::method_not_allowed, "invalidMethod", "Only GET method is expected");
        }

        if (req.target().starts_with("/api/v1/maps")) {
            if (req.target() == "/api/v1/maps") {
                json::array maps_json;
                for (const auto& map : game_.GetMaps()) {
                    maps_json.push_back({
                        {"id", *map.GetId()},
                        {"name", map.GetName()}
                        });
                }
                return text_response(http::status::ok, json::serialize(maps_json));
            }
            else {
                std::string map_id = req.target().substr(12).to_string();
                if (const auto* map = game_.FindMap(model::Map::Id{ map_id })) {
                    json::value map_json{
                        {"id", *map->GetId()},
                        {"name", map->GetName()},
                        {"roads", json::array()},
                        {"buildings", json::array()},
                        {"offices", json::array()}
                    };

                    for (const auto& road : map->GetRoads()) {
                        if (road.IsHorizontal()) {
                            map_json.as_object()["roads"].as_array().push_back({
                                {"x0", road.GetStart().x},
                                {"y0", road.GetStart().y},
                                {"x1", road.GetEnd().x}
                                });
                        }
                        else {
                            map_json.as_object()["roads"].as_array().push_back({
                                {"x0", road.GetStart().x},
                                {"y0", road.GetStart().y},
                                {"y1", road.GetEnd().y}
                                });
                        }
                    }

                    for (const auto& building : map->GetBuildings()) {
                        const auto& bounds = building.GetBounds();
                        map_json.as_object()["buildings"].as_array().push_back({
                            {"x", bounds.position.x},
                            {"y", bounds.position.y},
                            {"w", bounds.size.width},
                            {"h", bounds.size.height}
                            });
                    }

                    for (const auto& office : map->GetOffices()) {
                        map_json.as_object()["offices"].as_array().push_back({
                            {"id", *office.GetId()},
                            {"x", office.GetPosition().x},
                            {"y", office.GetPosition().y},
                            {"offsetX", office.GetOffset().dx},
                            {"offsetY", office.GetOffset().dy}
                            });
                    }

                    return text_response(http::status::ok, json::serialize(map_json));
                }
                else {
                    return error_response(http::status::not_found, "mapNotFound", "Map not found");
                }
            }
        }
        else if (req.target().starts_with("/api/")) {
            return error_response(http::status::bad_request, "badRequest", "Bad request");
        }

        return error_response(http::status::not_found, "notFound", "Not found");
    }

}  // namespace http_handler
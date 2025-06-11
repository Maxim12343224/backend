#include "request_handler.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    namespace {

        StringResponse MakeStringResponse(http::status status, std::string_view body,
            const StringRequest& req) {
            StringResponse response(status, req.version());
            response.set(http::field::content_type, "application/json");
            response.body() = body;
            response.prepare_payload();
            response.keep_alive(req.keep_alive());
            return response;
        }

        StringResponse MakeErrorResponse(http::status status, std::string_view code,
            std::string_view message, const StringRequest& req) {
            json::value json_res{
                {"code", code},
                {"message", message}
            };
            return MakeStringResponse(status, json::serialize(json_res), req);
        }

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

    StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
        if (req.method() != http::verb::get) {
            return MakeErrorResponse(http::status::method_not_allowed,
                "invalidMethod",
                "Only GET method is expected", req);
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
                return MakeStringResponse(http::status::ok,
                    json::serialize(maps_json), req);
            }

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

        if (req.target().starts_with("/api/")) {
            return MakeErrorResponse(http::status::bad_request,
                "badRequest",
                "Bad request", req);
        }

        return MakeErrorResponse(http::status::not_found,
            "notFound",
            "Not found", req);
    }

}  // namespace http_handler
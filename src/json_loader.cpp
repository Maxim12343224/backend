#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>

namespace json_loader {

    namespace json = boost::json;

    model::Game LoadGame(const std::filesystem::path& json_path) {
        std::ifstream file(json_path);
        if (!file) {
            throw std::runtime_error("Failed to open json file");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();

        auto value = json::parse(json_str);
        auto& maps = value.as_object().at("maps").as_array();

        model::Game game;

        for (const auto& map_val : maps) {
            auto& map_obj = map_val.as_object();

            model::Map::Id id{ std::string(map_obj.at("id").as_string().c_str()) };
            std::string name = std::string(map_obj.at("name").as_string().c_str());
            model::Map map(std::move(id), std::move(name));

            for (const auto& road_val : map_obj.at("roads").as_array()) {
                auto& road_obj = road_val.as_object();
                if (road_obj.contains("x1")) {
                    model::Point start{
                        static_cast<model::Coord>(road_obj.at("x0").as_int64()),
                        static_cast<model::Coord>(road_obj.at("y0").as_int64())
                    };
                    map.AddRoad(model::Road(model::Road::HORIZONTAL, start,
                        static_cast<model::Coord>(road_obj.at("x1").as_int64())));
                }
                else {
                    model::Point start{
                        static_cast<model::Coord>(road_obj.at("x0").as_int64()),
                        static_cast<model::Coord>(road_obj.at("y0").as_int64())
                    };
                    map.AddRoad(model::Road(model::Road::VERTICAL, start,
                        static_cast<model::Coord>(road_obj.at("y1").as_int64())));
                }
            }

            for (const auto& building_val : map_obj.at("buildings").as_array()) {
                auto& building_obj = building_val.as_object();
                model::Rectangle rect{
                    {
                        static_cast<model::Coord>(building_obj.at("x").as_int64()),
                        static_cast<model::Coord>(building_obj.at("y").as_int64())
                    },
                    {
                        static_cast<model::Dimension>(building_obj.at("w").as_int64()),
                        static_cast<model::Dimension>(building_obj.at("h").as_int64())
                    }
                };
                map.AddBuilding(model::Building(rect));
            }

            for (const auto& office_val : map_obj.at("offices").as_array()) {
                auto& office_obj = office_val.as_object();
                model::Office::Id id{ std::string(office_obj.at("id").as_string().c_str()) };
                model::Point pos{
                    static_cast<model::Coord>(office_obj.at("x").as_int64()),
                    static_cast<model::Coord>(office_obj.at("y").as_int64())
                };
                model::Offset offset{
                    static_cast<model::Dimension>(office_obj.at("offsetX").as_int64()),
                    static_cast<model::Dimension>(office_obj.at("offsetY").as_int64())
                };
                map.AddOffice(model::Office(std::move(id), pos, offset));
            }

            game.AddMap(std::move(map));
        }

        return game;
    }

}  // namespace json_loader
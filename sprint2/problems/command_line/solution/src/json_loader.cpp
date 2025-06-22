#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>
#include <utility>

using namespace std::literals;

namespace json_loader {

    namespace json = boost::json;

    namespace detail {

        model::Road ParseRoad(const json::object& road_obj) {
            if (road_obj.contains("x1")) {
                model::Point start{
                    static_cast<model::Coord>(road_obj.at("x0").as_int64()),
                    static_cast<model::Coord>(road_obj.at("y0").as_int64())
                };
                return model::Road(model::Road::HORIZONTAL, start,
                    static_cast<model::Coord>(road_obj.at("x1").as_int64()));
            }
            model::Point start{
                static_cast<model::Coord>(road_obj.at("x0").as_int64()),
                static_cast<model::Coord>(road_obj.at("y0").as_int64())
            };
            return model::Road(model::Road::VERTICAL, start,
                static_cast<model::Coord>(road_obj.at("y1").as_int64()));
        }

        model::Building ParseBuilding(const json::object& building_obj) {
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
            return model::Building(rect);
        }

        model::Office ParseOffice(const json::object& office_obj) {
            model::Office::Id id{ std::string(office_obj.at("id").as_string().c_str()) };
            model::Point pos{
                static_cast<model::Coord>(office_obj.at("x").as_int64()),
                static_cast<model::Coord>(office_obj.at("y").as_int64())
            };
            model::Offset offset{
                static_cast<model::Dimension>(office_obj.at("offsetX").as_int64()),
                static_cast<model::Dimension>(office_obj.at("offsetY").as_int64())
            };
            return model::Office(std::move(id), pos, offset);
        }

        model::Map ParseMap(const json::object& map_obj, double default_dog_speed) {
            model::Map::Id id{ std::string(map_obj.at("id").as_string().c_str()) };
            std::string name = std::string(map_obj.at("name").as_string().c_str());

            double dog_speed = default_dog_speed;
            if (map_obj.contains("dogSpeed")) {
                dog_speed = map_obj.at("dogSpeed").as_double();
            }

            model::Map map(std::move(id), std::move(name), dog_speed);

            for (const auto& road_val : map_obj.at("roads").as_array()) {
                map.AddRoad(ParseRoad(road_val.as_object()));
            }

            for (const auto& building_val : map_obj.at("buildings").as_array()) {
                map.AddBuilding(ParseBuilding(building_val.as_object()));
            }

            for (const auto& office_val : map_obj.at("offices").as_array()) {
                map.AddOffice(ParseOffice(office_val.as_object()));
            }

            return map;
        }

    } // namespace detail

    std::unique_ptr<model::Game> LoadGame(const std::filesystem::path& json_path) {
        std::ifstream file(json_path);
        if (!file) {
            throw std::runtime_error("Failed to open json file"s);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();

        try {
            auto value = json::parse(json_str);
            auto& maps = value.as_object().at("maps").as_array();

            double default_dog_speed = 1.0;
            if (value.as_object().contains("defaultDogSpeed")) {
                default_dog_speed = value.as_object().at("defaultDogSpeed").as_double();
            }

            auto game = std::make_unique<model::Game>(default_dog_speed);
            for (const auto& map_val : maps) {
                game->AddMap(detail::ParseMap(map_val.as_object(), game->GetDefaultDogSpeed()));
            }
            return game;
        }
        catch (const std::exception& e) {
            throw std::runtime_error("JSON parsing error: "s + e.what());
        }
    }

} // namespace json_loader
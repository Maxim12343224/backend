#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>
#include <string>

using namespace std::literals;

namespace json_loader {
    namespace json = boost::json;

    namespace {

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

        void ParseLootGeneratorConfig(const json::object& config_obj, model::Game& game) {
            if (!config_obj.contains("lootGeneratorConfig")) {
                return;
            }

            auto& loot_config = config_obj.at("lootGeneratorConfig").as_object();
            double period_seconds = loot_config.at("period").as_double();
            double probability = loot_config.at("probability").as_double();

            auto period_ms = std::chrono::milliseconds(static_cast<int>(period_seconds * 1000));
            game.SetLootGeneratorConfig(period_ms, probability);
        }

        void ParseDogRetirementTime(const json::object& config_obj, model::Game& game) {
            if (!config_obj.contains("dogRetirementTime")) {
                return;
            }

            double retirement_seconds = config_obj.at("dogRetirementTime").as_double();
            auto retirement_ms = std::chrono::milliseconds(static_cast<int>(retirement_seconds * 1000));
            game.SetRetirementTime(retirement_ms);
        }

        void ParseMapLootTypes(const json::object& map_obj, model::Game& game, const model::Map::Id& map_id) {
            if (!map_obj.contains("lootTypes")) {
                return;
            }

            auto& loot_types = map_obj.at("lootTypes").as_array();
            game.SetMapLootTypesCount(map_id, loot_types.size());
            
            // Извлекаем значения предметов
            std::vector<int> loot_values;
            for (const auto& loot_type : loot_types) {
                auto& loot_obj = loot_type.as_object();
                if (loot_obj.contains("value")) {
                    loot_values.push_back(static_cast<int>(loot_obj.at("value").as_int64()));
                } else {
                    loot_values.push_back(0);  // Значение по умолчанию
                }
            }
            
            game.SetMapLootValues(map_id, loot_values);
        }

        void ParseMapBagCapacity(const json::object& map_obj, model::Game& game, const model::Map::Id& map_id) {
            if (!map_obj.contains("bagCapacity")) {
                return;
            }

            size_t bag_capacity = static_cast<size_t>(map_obj.at("bagCapacity").as_int64());
            game.SetMapBagCapacity(map_id, bag_capacity);
        }

        model::Map ParseMap(const json::object& map_obj, model::Game& game) {
            model::Map::Id id{ std::string(map_obj.at("id").as_string().c_str()) };
            std::string name = std::string(map_obj.at("name").as_string().c_str());
            model::Map map(std::move(id), std::move(name));

            if (map_obj.contains("dogSpeed")) {
                map.SetDogSpeed(map_obj.at("dogSpeed").as_double());
            }

            for (const auto& road_val : map_obj.at("roads").as_array()) {
                map.AddRoad(ParseRoad(road_val.as_object()));
            }

            for (const auto& building_val : map_obj.at("buildings").as_array()) {
                map.AddBuilding(ParseBuilding(building_val.as_object()));
            }

            for (const auto& office_val : map_obj.at("offices").as_array()) {
                map.AddOffice(ParseOffice(office_val.as_object()));
            }

            ParseMapLootTypes(map_obj, game, map.GetId());
            ParseMapBagCapacity(map_obj, game, map.GetId());

            return map;
        }

    } // namespace

    void LoadGame(const std::filesystem::path& json_path, model::Game& game) {
        std::ifstream file(json_path);
        if (!file) {
            throw std::runtime_error("Failed to open json file");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();

        try {
            auto value = json::parse(json_str);
            auto& root = value.as_object();
            
            ParseLootGeneratorConfig(root, game);
            ParseDogRetirementTime(root, game);
            
            if (root.contains("defaultDogSpeed")) {
                game.SetDefaultDogSpeed(root.at("defaultDogSpeed").as_double());
            }

            if (root.contains("defaultBagCapacity")) {
                game.SetDefaultBagCapacity(static_cast<size_t>(root.at("defaultBagCapacity").as_int64()));
            }

            auto& maps = root.at("maps").as_array();
            for (const auto& map_val : maps) {
                game.AddMap(ParseMap(map_val.as_object(), game));
            }
        }
        catch (const std::exception& e) {
            throw std::runtime_error("JSON parsing error: "s + e.what());
        }
    }
}  // namespace json_loader
#include "serializer.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace serializer {

namespace json = boost::json;

// Сериализация DTO классов
json::value SerializePoint(const model::Point& point) {
    return json::array{point.x, point.y};
}

model::Point DeserializePoint(const json::value& point_val) {
    const auto& arr = point_val.as_array();
    return {arr[0].as_double(), arr[1].as_double()};
}

json::value SerializeDirection(model::Direction dir) {
    switch (dir) {
        case model::Direction::North: return "U";
        case model::Direction::South: return "D";
        case model::Direction::West: return "L";
        case model::Direction::East: return "R";
    }
    return "U";
}

model::Direction DeserializeDirection(const json::value& dir_val) {
    std::string dir_str = dir_val.as_string().c_str();
    if (dir_str == "U") return model::Direction::North;
    if (dir_str == "D") return model::Direction::South;
    if (dir_str == "L") return model::Direction::West;
    if (dir_str == "R") return model::Direction::East;
    return model::Direction::North;
}

// Сериализация DTO объектов
json::value SerializeDog(const SerDog& dog) {
    return {
        {"name", dog.name},
        {"position", SerializePoint(dog.position)},
        {"speed", SerializePoint(dog.speed)},
        {"direction", SerializeDirection(dog.direction)}
    };
}

SerDog DeserializeDog(const json::value& dog_val) {
    const auto& obj = dog_val.as_object();
    return {
        obj.at("name").as_string().c_str(),
        DeserializePoint(obj.at("position")),
        DeserializePoint(obj.at("speed")),
        DeserializeDirection(obj.at("direction"))
    };
}

json::value SerializeLostObject(const SerLostObject& obj) {
    return {
        {"id", obj.id},
        {"type", obj.type},
        {"position", SerializePoint(obj.position)},
        {"value", obj.value}
    };
}

SerLostObject DeserializeLostObject(const json::value& obj_val) {
    const auto& obj = obj_val.as_object();
    return {
        obj.at("id").as_uint64(),
        obj.at("type").as_uint64(),
        DeserializePoint(obj.at("position")),
        static_cast<int>(obj.at("value").as_int64())
    };
}

json::value SerializePlayer(const SerPlayer& player) {
    json::array bag_json;
    for (const auto& item : player.bag) {
        bag_json.push_back(SerializeLostObject(item));
    }
    
    return {
        {"id", player.id},
        {"token", player.token},
        {"dog", SerializeDog(player.dog)},
        {"score", player.score},
        {"bag_capacity", player.bag_capacity},
        {"bag", bag_json}
    };
}

SerPlayer DeserializePlayer(const json::value& player_val) {
    const auto& obj = player_val.as_object();
    
    std::vector<SerLostObject> bag;
    const auto& bag_arr = obj.at("bag").as_array();
    for (const auto& item_val : bag_arr) {
        bag.push_back(DeserializeLostObject(item_val));
    }
    
    return {
        obj.at("id").as_uint64(),
        obj.at("token").as_string().c_str(),
        DeserializeDog(obj.at("dog")),
        obj.at("score").as_int64(),
        obj.at("bag_capacity").as_uint64(),
        bag
    };
}

json::value SerializeSession(const SerGameSession& session) {
    json::array players_json;
    for (const auto& player : session.players) {
        players_json.push_back(SerializePlayer(player));
    }
    
    json::array lost_objects_json;
    for (const auto& obj : session.lost_objects) {
        lost_objects_json.push_back(SerializeLostObject(obj));
    }
    
    return {
        {"id", session.id},
        {"map_id", session.map_id},
        {"dog_speed", session.dog_speed},
        {"next_lost_object_id", session.next_lost_object_id},
        {"players", players_json},
        {"lost_objects", lost_objects_json}
    };
}

SerGameSession DeserializeSession(const json::value& session_val) {
    const auto& obj = session_val.as_object();
    
    std::vector<SerPlayer> players;
    const auto& players_arr = obj.at("players").as_array();
    for (const auto& player_val : players_arr) {
        players.push_back(DeserializePlayer(player_val));
    }
    
    std::vector<SerLostObject> lost_objects;
    const auto& objects_arr = obj.at("lost_objects").as_array();
    for (const auto& obj_val : objects_arr) {
        lost_objects.push_back(DeserializeLostObject(obj_val));
    }
    
    return {
        obj.at("id").as_uint64(),
        obj.at("map_id").as_string().c_str(),
        obj.at("dog_speed").as_double(),
        obj.at("next_lost_object_id").as_uint64(),
        players,
        lost_objects
    };
}

// Основные методы сериализации/десериализации игры
json::value GameSerializer::SerializeGame(const model::Game& game) {
    json::object state;
    
    state["default_dog_speed"] = game.GetDefaultDogSpeed();
    state["default_bag_capacity"] = game.GetDefaultBagCapacity();
    
    json::array sessions_json;
    const auto& maps = game.GetMaps();
    for (const auto& map : maps) {
        if (auto session = const_cast<model::Game&>(game).FindSession(map.GetId())) {
            sessions_json.push_back(SerializeSession(SerGameSession::FromModel(session)));
        }
    }
    state["sessions"] = sessions_json;
    
    json::object token_map;
    auto players = game.GetAllPlayers();
    for (const auto& player : players) {
        token_map[std::string(*player->GetToken())] = *player->GetId();
    }
    state["token_to_player"] = token_map;
    
    return state;
}

void GameSerializer::DeserializeGame(model::Game& game, const json::value& data) {
    if (!data.is_object()) {
        throw std::runtime_error("Invalid state format");
    }
    
    const auto& obj = data.as_object();
    
    if (obj.contains("default_dog_speed")) {
        game.SetDefaultDogSpeed(obj.at("default_dog_speed").as_double());
    }
    
    if (obj.contains("default_bag_capacity")) {
        game.SetDefaultBagCapacity(obj.at("default_bag_capacity").as_uint64());
    }
    
    if (obj.contains("sessions")) {
        const auto& sessions_arr = obj.at("sessions").as_array();
        for (const auto& session_val : sessions_arr) {
            auto ser_session = DeserializeSession(session_val);
            
            const auto* map = game.FindMap(model::Map::Id{ser_session.map_id});
            if (!map) {
                throw std::runtime_error("Map not found: " + ser_session.map_id);
            }
            
            auto session = std::make_shared<model::GameSession>(
                *map, ser_session.id, ser_session.dog_speed, false, 
                game.GetLootGenerator(), game.GetMapBagCapacity(model::Map::Id{ser_session.map_id})
            );
            
            session->SetLootTypesCount(game.GetMapLootTypesCount(model::Map::Id{ser_session.map_id}));
            session->SetLootValues(game.GetMapLootValues(model::Map::Id{ser_session.map_id}));
            session->SetNextLostObjectId(ser_session.next_lost_object_id);
            
            // Восстанавливаем игроков
            for (const auto& ser_player : ser_session.players) {
                auto dog = ser_player.dog.ToModel();
                auto player = std::make_shared<model::Player>(
                    session, std::move(dog), ser_player.id, ser_player.token, ser_player.bag_capacity
                );
                
                player->AddScore(ser_player.score);
                
                for (const auto& ser_item : ser_player.bag) {
                    player->AddItemToBag(ser_item.ToModel());
                }
                
                session->AddRestoredPlayer(player);
            }
            
            // Восстанавливаем потерянные предметы
            for (const auto& ser_obj : ser_session.lost_objects) {
                session->AddRestoredLostObject(ser_obj.ToModel());
            }
            
            game.AddRestoredSession(model::Map::Id{ser_session.map_id}, session);
        }
    }
}

bool GameSerializer::SaveToFile(const model::Game& game, const std::filesystem::path& path) {
    try {
        if (path.empty()) {
            return false;
        }

        auto temp_path = path;
        temp_path += ".tmp";
        
        auto parent_dir = temp_path.parent_path();
        if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
            std::filesystem::create_directories(parent_dir);
        }
        
        std::ofstream file(temp_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        auto state = SerializeGame(game);
        std::string json_str = json::serialize(state);
        file.write(json_str.c_str(), json_str.size());
        
        if (!file.good()) {
            file.close();
            std::filesystem::remove(temp_path);
            return false;
        }
        
        file.close();
        
        std::filesystem::rename(temp_path, path);
        
        return true;
    } catch (const std::exception& e) {
        try {
            std::filesystem::remove(path.string() + ".tmp");
        } catch (...) {
        }
        return false;
    }
}

bool GameSerializer::LoadFromFile(model::Game& game, const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();
        
        if (json_str.empty()) {
            return false;
        }
        
        auto state = json::parse(json_str);
        DeserializeGame(game, state);
        
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load state: " + std::string(e.what()));
    }
}

} // namespace serializer
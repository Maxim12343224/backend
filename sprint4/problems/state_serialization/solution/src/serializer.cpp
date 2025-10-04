#include "serializer.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace serializer {

namespace json = boost::json;

json::value SerializePoint(const model::Point& point) {
    return json::array{point.x, point.y};
}

model::Point DeserializePoint(const json::value& point_val) {
    const auto& arr = point_val.as_array();
    return {arr[0].as_double(), arr[1].as_double()};
}

json::value SerializeDog(const model::Dog& dog) {
    return {
        {"name", dog.GetName()},
        {"position", SerializePoint(dog.GetPosition())},
        {"speed", SerializePoint(dog.GetSpeed())},
        {"direction", static_cast<int>(dog.GetDirection())}
    };
}

model::Dog DeserializeDog(const json::value& dog_val) {
    const auto& obj = dog_val.as_object();
    std::string name = obj.at("name").as_string().c_str();
    model::Point position = DeserializePoint(obj.at("position"));
    
    model::Dog dog(name, position);
    dog.SetSpeed(DeserializePoint(obj.at("speed")));
    dog.SetDirection(static_cast<model::Direction>(obj.at("direction").as_uint64()));
    
    return dog;
}

json::value SerializeLostObject(const model::LostObject& obj) {
    return {
        {"id", obj.id},
        {"type", obj.type},
        {"position", SerializePoint(obj.position)},
        {"value", obj.value}
    };
}

model::LostObject DeserializeLostObject(const json::value& obj_val) {
    const auto& obj = obj_val.as_object();
    return {
        obj.at("id").as_uint64(),
        obj.at("type").as_uint64(),
        DeserializePoint(obj.at("position")),
        static_cast<int>(obj.at("value").as_int64())
    };
}

json::value SerializePlayer(const std::shared_ptr<model::Player>& player) {
    json::array bag_json;
    for (const auto& item : player->GetBag()) {
        bag_json.push_back(SerializeLostObject(item));
    }
    
    return {
        {"id", *player->GetId()},
        {"token", *player->GetToken()},
        {"dog", SerializeDog(player->GetDog())},
        {"score", player->GetScore()},
        {"bag_capacity", player->GetBagCapacity()},
        {"bag", bag_json}
    };
}

std::shared_ptr<model::Player> DeserializePlayer(
    const json::value& player_val, 
    std::shared_ptr<model::GameSession> session) {
    
    const auto& obj = player_val.as_object();
    
    uint32_t id = obj.at("id").as_uint64();
    std::string token = obj.at("token").as_string().c_str();
    model::Dog dog = DeserializeDog(obj.at("dog"));
    int score = obj.at("score").as_int64();
    size_t bag_capacity = obj.at("bag_capacity").as_uint64();
    
    auto player = std::make_shared<model::Player>(
        session, std::move(dog), id, token, bag_capacity
    );
    
    player->AddScore(score);
    
    const auto& bag_arr = obj.at("bag").as_array();
    for (const auto& item_val : bag_arr) {
        player->AddItemToBag(DeserializeLostObject(item_val));
    }
    
    return player;
}

json::value SerializeSession(const std::shared_ptr<model::GameSession>& session) {
    json::array players_json;
    for (const auto& player : session->GetPlayers()) {
        players_json.push_back(SerializePlayer(player));
    }
    
    json::array lost_objects_json;
    for (const auto& obj : session->GetLostObjects()) {
        lost_objects_json.push_back(SerializeLostObject(obj));
    }
    
    return {
        {"id", *session->GetId()},
        {"map_id", *session->GetMap().GetId()},
        {"dog_speed", session->GetDogSpeed()},
        {"next_lost_object_id", session->GetNextLostObjectId()},
        {"players", players_json},
        {"lost_objects", lost_objects_json}
    };
}

void DeserializeSession(model::Game& game, const json::value& session_val) {
    const auto& obj = session_val.as_object();
    
    uint32_t session_id = obj.at("id").as_uint64();
    std::string map_id = obj.at("map_id").as_string().c_str();
    double dog_speed = obj.at("dog_speed").as_double();
    size_t next_lost_object_id = obj.at("next_lost_object_id").as_uint64();
    
    const auto* map = game.FindMap(model::Map::Id{map_id});
    if (!map) {
        throw std::runtime_error("Map not found: " + map_id);
    }
    
    auto session = std::make_shared<model::GameSession>(
        *map, session_id, dog_speed, false, game.GetLootGenerator(), 
        game.GetMapBagCapacity(model::Map::Id{map_id})
    );
    
    session->SetLootTypesCount(game.GetMapLootTypesCount(model::Map::Id{map_id}));
    session->SetLootValues(game.GetMapLootValues(model::Map::Id{map_id}));
    session->SetNextLostObjectId(next_lost_object_id);
    
    const auto& players_arr = obj.at("players").as_array();
    for (const auto& player_val : players_arr) {
        auto player = DeserializePlayer(player_val, session);
        session->AddRestoredPlayer(player);
    }
    
    const auto& objects_arr = obj.at("lost_objects").as_array();
    for (const auto& obj_val : objects_arr) {
        session->AddRestoredLostObject(DeserializeLostObject(obj_val));
    }
    
    game.AddRestoredSession(model::Map::Id{map_id}, session);
}

json::value GameSerializer::SerializeGame(const model::Game& game) {
    json::object state;
    
    state["default_dog_speed"] = game.GetDefaultDogSpeed();
    state["default_bag_capacity"] = game.GetDefaultBagCapacity();
    state["randomize_spawn_points"] = false;
    
    json::array sessions_json;
    const auto& maps = game.GetMaps();
    for (const auto& map : maps) {
        if (auto session = const_cast<model::Game&>(game).FindSession(map.GetId())) {
            sessions_json.push_back(SerializeSession(session));
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
            DeserializeSession(game, session_val);
        }
    }
}

bool GameSerializer::SaveToFile(const model::Game& game, const std::filesystem::path& path) {
    std::cout << "DEBUG: GameSerializer::SaveToFile - path: " << path.string() << std::endl;
    
    try {
        if (path.empty()) {
            std::cout << "DEBUG: SaveToFile - empty path, returning false" << std::endl;
            return false;
        }

        auto temp_path = path;
        temp_path += ".tmp";
        
        std::cout << "DEBUG: SaveToFile - temp path: " << temp_path.string() << std::endl;
        
        // Создаем директорию, если не существует
        auto parent_dir = temp_path.parent_path();
        if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
            std::cout << "DEBUG: SaveToFile - creating directory: " << parent_dir.string() << std::endl;
            std::filesystem::create_directories(parent_dir);
        }
        
        std::cout << "DEBUG: SaveToFile - opening file: " << temp_path.string() << std::endl;
        std::ofstream file(temp_path, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "DEBUG: SaveToFile - failed to open file" << std::endl;
            return false;
        }
        
        auto state = SerializeGame(game);
        std::string json_str = json::serialize(state);
        
        std::cout << "DEBUG: SaveToFile - writing " << json_str.size() << " bytes" << std::endl;
        file.write(json_str.c_str(), json_str.size());
        
        if (!file.good()) {
            std::cout << "DEBUG: SaveToFile - file write failed" << std::endl;
            file.close();
            std::filesystem::remove(temp_path);
            return false;
        }
        
        file.close();
        
        std::cout << "DEBUG: SaveToFile - renaming " << temp_path.string() << " to " << path.string() << std::endl;
        
        // Атомарная замена файла
        std::filesystem::rename(temp_path, path);
        
        std::cout << "DEBUG: SaveToFile - SUCCESS" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "DEBUG: SaveToFile - EXCEPTION: " << e.what() << std::endl;
        // Удаляем временный файл в случае ошибки
        try {
            auto temp_path = path.string() + ".tmp";
            if (std::filesystem::exists(temp_path)) {
                std::filesystem::remove(temp_path);
            }
        } catch (...) {
            // Игнорируем ошибки удаления
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
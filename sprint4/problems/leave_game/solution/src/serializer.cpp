#include "serializer.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace serializer {

namespace json = boost::json;

// Безопасные методы для чтения чисел из JSON
uint32_t SafeGetUint32(const json::value& value) {
    if (value.is_uint64()) {
        return static_cast<uint32_t>(value.as_uint64());
    } else if (value.is_int64()) {
        return static_cast<uint32_t>(value.as_int64());
    } else if (value.is_double()) {
        return static_cast<uint32_t>(value.as_double());
    } else {
        throw std::runtime_error("Value is not a number");
    }
}

size_t SafeGetUint64(const json::value& value) {
    if (value.is_uint64()) {
        return static_cast<size_t>(value.as_uint64());
    } else if (value.is_int64()) {
        return static_cast<size_t>(value.as_int64());
    } else if (value.is_double()) {
        return static_cast<size_t>(value.as_double());
    } else {
        throw std::runtime_error("Value is not a number");
    }
}

int SafeGetInt(const json::value& value) {
    if (value.is_int64()) {
        return static_cast<int>(value.as_int64());
    } else if (value.is_uint64()) {
        return static_cast<int>(value.as_uint64());
    } else if (value.is_double()) {
        return static_cast<int>(value.as_double());
    } else {
        throw std::runtime_error("Value is not a number");
    }
}

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
        SafeGetUint64(obj.at("id")),
        SafeGetUint64(obj.at("type")),
        DeserializePoint(obj.at("position")),
        SafeGetInt(obj.at("value"))
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
        SafeGetUint32(obj.at("id")),
        std::string(obj.at("token").as_string()),
        DeserializeDog(obj.at("dog")),
        SafeGetInt(obj.at("score")),
        SafeGetUint64(obj.at("bag_capacity")),
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
        SafeGetUint32(obj.at("id")),
        std::string(obj.at("map_id").as_string()),
        obj.at("dog_speed").as_double(),
        SafeGetUint64(obj.at("next_lost_object_id")),
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
    
    // Получаем все сессии
    std::vector<std::shared_ptr<model::GameSession>> all_sessions;
    {
        auto& mutable_game = const_cast<model::Game&>(game);
        auto maps = game.GetMaps();
        for (const auto& map : maps) {
            if (auto session = mutable_game.FindSession(map.GetId())) {
                all_sessions.push_back(session);
            }
        }
    }
    
    for (const auto& session : all_sessions) {
        sessions_json.push_back(SerializeSession(SerGameSession::FromModel(session)));
    }
    state["sessions"] = sessions_json;
    
    // Сериализуем mapping токенов
    json::object token_map;
    auto players = game.GetAllPlayers();
    for (const auto& player : players) {
        token_map[std::string(*player->GetToken())] = *player->GetId();
    }
    state["token_to_player"] = token_map;
    
    // Сохраняем время выхода на пенсию
    auto retirement_time = std::chrono::duration_cast<std::chrono::milliseconds>(game.GetRetirementTime());
    state["retirement_time_ms"] = retirement_time.count();
    
    return state;
}

void GameSerializer::DeserializeGame(model::Game& game, const json::value& data) {
    std::cout << "=== DEBUG: DeserializeGame START ===" << std::endl;
    
    if (!data.is_object()) {
        std::cout << "ERROR: Data is not an object" << std::endl;
        throw std::runtime_error("Invalid state format");
    }
    
    const auto& obj = data.as_object();
    std::cout << "DEBUG: JSON object has " << obj.size() << " fields" << std::endl;

    try {
        if (obj.contains("default_dog_speed")) {
            game.SetDefaultDogSpeed(obj.at("default_dog_speed").as_double());
            std::cout << "DEBUG: Set default_dog_speed: " << obj.at("default_dog_speed").as_double() << std::endl;
        }

        if (obj.contains("default_bag_capacity")) {
            game.SetDefaultBagCapacity(SafeGetUint64(obj.at("default_bag_capacity")));
            std::cout << "DEBUG: Set default_bag_capacity: " << SafeGetUint64(obj.at("default_bag_capacity")) << std::endl;
        }

        // Восстанавливаем время выхода на пенсию
        if (obj.contains("retirement_time_ms")) {
            auto retirement_time = std::chrono::milliseconds(SafeGetUint64(obj.at("retirement_time_ms")));
            game.SetRetirementTime(retirement_time);
            std::cout << "DEBUG: Set retirement_time: " << retirement_time.count() << "ms" << std::endl;
        }

        // Создаем временный mapping для восстановления токенов
        std::unordered_map<uint32_t, std::shared_ptr<model::Player>> player_id_to_player;
        std::vector<std::pair<std::string, std::shared_ptr<model::GameSession>>> sessions_to_add;
        
        if (obj.contains("sessions")) {
            const auto& sessions_arr = obj.at("sessions").as_array();
            std::cout << "DEBUG: Found " << sessions_arr.size() << " sessions" << std::endl;
            
            for (size_t session_idx = 0; session_idx < sessions_arr.size(); ++session_idx) {
                std::cout << "DEBUG: Processing session " << session_idx << std::endl;
                const auto& session_val = sessions_arr[session_idx];
                
                auto ser_session = DeserializeSession(session_val);
                std::cout << "DEBUG: Session ID: " << ser_session.id << ", Map ID: " << ser_session.map_id 
                          << ", Players: " << ser_session.players.size() 
                          << ", Lost objects: " << ser_session.lost_objects.size() << std::endl;

                const auto* map = game.FindMap(model::Map::Id{ser_session.map_id});
                if (!map) {
                    std::cout << "ERROR: Map not found: " << ser_session.map_id << std::endl;
                    throw std::runtime_error("Map not found: " + ser_session.map_id);
                }

                // Создаем сессию с временем выхода на пенсию
                auto session = std::make_shared<model::GameSession>(
                    *map, ser_session.id, ser_session.dog_speed, false, 
                    game.GetLootGenerator(), game.GetMapBagCapacity(model::Map::Id{ser_session.map_id}),
                    game.GetRetirementTime()
                );

                // Настраиваем callback для уведомления об ушедших игроках
                session->SetRetiredPlayersCallback(
                    [&game](const auto& retired_players) {
                        game.HandlePlayerRetirement(retired_players);
                    }
                );

                session->SetLootTypesCount(game.GetMapLootTypesCount(model::Map::Id{ser_session.map_id}));
                session->SetLootValues(game.GetMapLootValues(model::Map::Id{ser_session.map_id}));
                session->SetNextLostObjectId(ser_session.next_lost_object_id);

                // Восстанавливаем игроков
                std::cout << "DEBUG: Restoring " << ser_session.players.size() << " players" << std::endl;
                for (size_t player_idx = 0; player_idx < ser_session.players.size(); ++player_idx) {
                    const auto& ser_player = ser_session.players[player_idx];
                    std::cout << "DEBUG: Player " << player_idx << " - ID: " << ser_player.id 
                              << ", Token: " << ser_player.token << std::endl;

                    auto dog = ser_player.dog.ToModel();
                    auto player = std::make_shared<model::Player>(
                        session, std::move(dog), ser_player.id, ser_player.token, ser_player.bag_capacity
                    );

                    // Устанавливаем временные метки
                    player->SetJoinTime(0.0);  // Время входа будет установлено позже
                    player->SetLastMoveTime(0.0);  // Время последнего движения
                    
                    player->AddScore(ser_player.score);

                    for (const auto& ser_item : ser_player.bag) {
                        player->AddItemToBag(ser_item.ToModel());
                    }

                    session->AddRestoredPlayer(player);
                    player_id_to_player[ser_player.id] = player;
                    
                    std::cout << "DEBUG: Created player - ID: " << ser_player.id 
                              << ", Token: " << ser_player.token 
                              << ", Stored in mapping: " << (player_id_to_player.count(ser_player.id) > 0) << std::endl;
                }

                // Восстанавливаем потерянные предметы
                for (const auto& ser_obj : ser_session.lost_objects) {
                    session->AddRestoredLostObject(ser_obj.ToModel());
                }

                sessions_to_add.emplace_back(ser_session.map_id, session);
                std::cout << "DEBUG: Session " << session_idx << " prepared for addition" << std::endl;
            }
        } else {
            std::cout << "DEBUG: No sessions found in JSON" << std::endl;
        }

        // Восстанавливаем mapping токенов
        if (obj.contains("token_to_player")) {
            const auto& token_map = obj.at("token_to_player").as_object();
            std::cout << "DEBUG: Found token_to_player with " << token_map.size() << " entries" << std::endl;
            
            size_t restored_count = 0;
            size_t missing_count = 0;
            
            for (const auto& [token_str, player_id_val] : token_map) {
                uint32_t player_id = SafeGetUint32(player_id_val);
                auto it = player_id_to_player.find(player_id);
                
                if (it != player_id_to_player.end()) {
                    game.RestoreTokenToPlayerMapping(
                        model::Player::Token{std::string(token_str)}, 
                        it->second
                    );
                    std::cout << "DEBUG: SUCCESS - Restored token: '" << token_str 
                              << "' -> player " << player_id << std::endl;
                    restored_count++;
                } else {
                    std::cout << "DEBUG: ERROR - Player not found for token: '" << token_str 
                              << "', player ID: " << player_id << std::endl;
                    missing_count++;
                }
            }
            
            std::cout << "DEBUG: Token mapping summary - Restored: " << restored_count 
                      << ", Missing: " << missing_count << std::endl;
        } else {
            std::cout << "DEBUG: WARNING - No token_to_player found in saved state" << std::endl;
        }

        // Добавляем сессии в игру
        std::cout << "DEBUG: Adding " << sessions_to_add.size() << " sessions to game" << std::endl;
        for (auto& [map_id, session] : sessions_to_add) {
            game.AddRestoredSession(model::Map::Id{map_id}, session);
            std::cout << "DEBUG: Added session for map: " << map_id << std::endl;
        }

        std::cout << "DEBUG: DeserializeGame completed. Total players in mapping: " 
                  << player_id_to_player.size() << std::endl;
        std::cout << "=== DEBUG: DeserializeGame SUCCESS ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "DEBUG: Exception in DeserializeGame: " << e.what() << std::endl;
        throw;
    }
}

bool GameSerializer::SaveToFile(const model::Game& game, const std::filesystem::path& path) {
    std::cout << "=== DEBUG: SaveToFile START ===" << std::endl;
    std::cout << "DEBUG: Target path: " << path << std::endl;
    
    try {
        if (path.empty()) {
            std::cout << "DEBUG: Path is empty - returning false" << std::endl;
            return false;
        }

        // Создаем временный файл
        auto temp_path = path;
        temp_path += ".tmp";
        std::cout << "DEBUG: Temp path: " << temp_path << std::endl;
        
        // Проверяем и создаем родительскую директорию
        auto parent_dir = temp_path.parent_path();
        std::cout << "DEBUG: Parent directory: " << parent_dir << std::endl;
        
        if (!parent_dir.empty()) {
            if (!std::filesystem::exists(parent_dir)) {
                std::cout << "DEBUG: Parent directory doesn't exist, creating..." << std::endl;
                if (!std::filesystem::create_directories(parent_dir)) {
                    std::cout << "DEBUG: Failed to create parent directory - returning false" << std::endl;
                    return false;
                }
                std::cout << "DEBUG: Parent directory created successfully" << std::endl;
            } else {
                std::cout << "DEBUG: Parent directory exists" << std::endl;
            }
        }
        
        // Открываем временный файл для записи
        std::cout << "DEBUG: Opening temp file for writing..." << std::endl;
        std::ofstream file(temp_path, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "DEBUG: Failed to open temp file - returning false" << std::endl;
            std::cout << "DEBUG: Error: " << strerror(errno) << std::endl;
            return false;
        }
        std::cout << "DEBUG: Temp file opened successfully" << std::endl;
        
        // Сериализуем состояние игры
        std::cout << "DEBUG: Serializing game state..." << std::endl;
        auto state = SerializeGame(game);
        std::string json_str = json::serialize(state);
        std::cout << "DEBUG: JSON data size: " << json_str.size() << " bytes" << std::endl;
        
        // Записываем данные во временный файл
        std::cout << "DEBUG: Writing data to temp file..." << std::endl;
        file.write(json_str.c_str(), json_str.size());
        
        // Проверяем успешность записи
        if (!file.good()) {
            std::cout << "DEBUG: Write operation failed - cleaning up and returning false" << std::endl;
            file.close();
            if (std::filesystem::exists(temp_path)) {
                std::filesystem::remove(temp_path);
            }
            return false;
        }
        
        file.close();
        std::cout << "DEBUG: Data written successfully to temp file" << std::endl;
        
        // Проверяем, что временный файл создан
        if (!std::filesystem::exists(temp_path)) {
            std::cout << "DEBUG: Temp file doesn't exist after writing - returning false" << std::endl;
            return false;
        }
        
        auto temp_file_size = std::filesystem::file_size(temp_path);
        std::cout << "DEBUG: Temp file size: " << temp_file_size << " bytes" << std::endl;
        
        // Атомарно переименовываем временный файл в целевой
        std::cout << "DEBUG: Renaming temp file to target..." << std::endl;
        std::filesystem::rename(temp_path, path);
        std::cout << "DEBUG: File renamed successfully" << std::endl;
        
        // Проверяем, что целевой файл создан
        if (!std::filesystem::exists(path)) {
            std::cout << "DEBUG: Target file doesn't exist after rename - returning false" << std::endl;
            return false;
        }
        
        auto target_file_size = std::filesystem::file_size(path);
        std::cout << "DEBUG: Target file size: " << target_file_size << " bytes" << std::endl;
        
        std::cout << "=== DEBUG: SaveToFile SUCCESS ===" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "DEBUG: Exception in SaveToFile: " << e.what() << std::endl;
        try {
            // Пытаемся удалить временный файл в случае ошибки
            if (std::filesystem::exists(path.string() + ".tmp")) {
                std::filesystem::remove(path.string() + ".tmp");
            }
        } catch (const std::exception& remove_ex) {
            std::cout << "DEBUG: Failed to remove temp file: " << remove_ex.what() << std::endl;
        }
        return false;
    }
}

bool GameSerializer::LoadFromFile(model::Game& game, const std::filesystem::path& path) {
    std::cout << "=== DEBUG: LoadFromFile START ===" << std::endl;
    std::cout << "DEBUG: Loading from path: " << path << std::endl;
    
    try {
        if (!std::filesystem::exists(path)) {
            std::cout << "DEBUG: File does not exist - returning false" << std::endl;
            return false;
        }
        
        auto file_size = std::filesystem::file_size(path);
        std::cout << "DEBUG: File exists, size: " << file_size << " bytes" << std::endl;
        
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "DEBUG: Failed to open file - returning false" << std::endl;
            return false;
        }
        std::cout << "DEBUG: File opened successfully" << std::endl;
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();
        
        if (json_str.empty()) {
            std::cout << "DEBUG: File is empty - returning false" << std::endl;
            return false;
        }
        
        std::cout << "DEBUG: JSON content size: " << json_str.size() << " bytes" << std::endl;
        std::cout << "DEBUG: First 100 chars: " << json_str.substr(0, 100) << std::endl;
        
        auto state = json::parse(json_str);
        DeserializeGame(game, state);
        
        std::cout << "=== DEBUG: LoadFromFile SUCCESS ===" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "DEBUG: Exception in LoadFromFile: " << e.what() << std::endl;
        throw std::runtime_error("Failed to load state: " + std::string(e.what()));
    }
}

} // namespace serializer
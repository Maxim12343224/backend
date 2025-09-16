#include "model.h"
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

/*std::shared_ptr<Player> Game::JoinGame(const Map::Id& map_id, std::string dog_name) {
    size_t loot_types_count = 0;
    const Map* map_ptr = nullptr;
    std::shared_ptr<GameSession> session;
    
    {
        map_ptr = FindMap(map_id);
        if (!map_ptr) {
            return nullptr;
        }
        
        loot_types_count = GetMapLootTypesCount(map_id);
        
        if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
            session = it->second;
        }
    }
    
    if (!session) {
        double dog_speed = map_ptr->GetDogSpeed().value_or(default_dog_speed_);
        session = std::make_shared<GameSession>(*map_ptr, next_session_id_++, dog_speed,
            randomize_spawn_points_, loot_generator_);
        session->SetLootTypesCount(loot_types_count);

        if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
            session = it->second;
        } else {
            map_id_to_session_[map_id] = session;
        }
    }

    auto player = session->AddPlayer(std::move(dog_name));
    if (!player) {
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);  // Добавить защиту
        token_to_player_[player->GetToken()] = player;
        std::cout << "DEBUG: Saved token: " << *player->GetToken() << std::endl;
        std::cout << "DEBUG: Map size: " << token_to_player_.size() << std::endl;
    }
    
    return player;
}*/











/*std::shared_ptr<Player> Game::JoinGame(const Map::Id& map_id, std::string dog_name) {
    // Временно уберем сложную логику с сессиями
    static std::atomic<uint32_t> next_id{0};
    
    auto map_ptr = FindMap(map_id);
    if (!map_ptr) return nullptr;
    
    // Просто создаем игрока без сложной логики сессий
    auto session = std::make_shared<GameSession>(*map_ptr, 0, 1.0, false, loot_generator_);
    auto player = session->AddPlayer(std::move(dog_name));
    
    if (!player) return nullptr;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        token_to_player_[player->GetToken()] = player;
        std::cout << "JOIN: Token=" << *player->GetToken() << " ID=" << *player->GetId() << std::endl;
    }
    
    return player;
}*/






std::shared_ptr<Player> Game::JoinGame(const Map::Id& map_id, std::string dog_name) {
    std::cout << "=== JoinGame START ===" << std::endl;
    std::cout << "Map ID: " << *map_id << std::endl;
    
    const Map* map_ptr = nullptr;
    size_t loot_types_count = 0;
    
    {
        std::cout << "Before mutex lock" << std::endl;
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "After mutex lock" << std::endl;
        
        map_ptr = FindMap(map_id);
        if (!map_ptr) {
            std::cout << "JoinGame ERROR: Map not found" << std::endl;
            return nullptr;
        }
        std::cout << "Map found: " << *map_ptr->GetId() << std::endl;
        
        loot_types_count = GetMapLootTypesCount(map_id);
        std::cout << "Loot types count: " << loot_types_count << std::endl;
    }
    
    std::shared_ptr<GameSession> session;
    {
        std::cout << "Before session mutex lock" << std::endl;
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "After session mutex lock" << std::endl;
        
        // Проверяем существующую сессию
        if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
            session = it->second;
            std::cout << "Using existing session: " << *session->GetId() << std::endl;
        } else {
            std::cout << "Creating new session..." << std::endl;
            double dog_speed = map_ptr->GetDogSpeed().value_or(default_dog_speed_);
            std::cout << "Dog speed: " << dog_speed << std::endl;
            
            session = std::make_shared<GameSession>(*map_ptr, next_session_id_++, dog_speed,
                randomize_spawn_points_, loot_generator_);
            std::cout << "Session created: " << *session->GetId() << std::endl;
            
            session->SetLootTypesCount(loot_types_count);
            std::cout << "Loot types set" << std::endl;
            
            // ДОБАВЛЯЕМ СЕССИЮ В MAP!
            map_id_to_session_[map_id] = session;
            std::cout << "Session added to map: " << *map_id << std::endl;
        }
    }
    
    std::cout << "Adding player to session..." << std::endl;
    auto player = session->AddPlayer(std::move(dog_name));
    if (!player) {
        std::cout << "JoinGame ERROR: Failed to add player" << std::endl;
        return nullptr;
    }
    std::cout << "Player added: " << *player->GetId() << std::endl;
    
    {
        std::cout << "Before token mutex lock" << std::endl;
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "After token mutex lock" << std::endl;
        
        token_to_player_[player->GetToken()] = player;
        std::cout << "Token saved: " << *player->GetToken() << std::endl;
    }
    
    std::cout << "=== JoinGame END ===" << std::endl;
    return player;
}







Point GameSession::GenerateRandomPosition() const {
    const auto& roads = map_.GetRoads();
    if (roads.empty()) return {0.0, 0.0};
    
    if (!randomize_spawn_points_) {
        const auto& road = roads[0];
        return {
            static_cast<double>(road.GetStart().x),
            static_cast<double>(road.GetStart().y)
        };
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
    const auto& road = roads[road_dist(gen)];

    if (road.IsHorizontal()) {
        std::uniform_real_distribution<double> x_dist(
            std::min(road.GetStart().x, road.GetEnd().x),
            std::max(road.GetStart().x, road.GetEnd().x)
        );
        return {x_dist(gen), static_cast<double>(road.GetStart().y)};
    } else {
        std::uniform_real_distribution<double> y_dist(
            std::min(road.GetStart().y, road.GetEnd().y),
            std::max(road.GetStart().y, road.GetEnd().y)
        );
        return {static_cast<double>(road.GetStart().x), y_dist(gen)};
    }
}

std::shared_ptr<Player> GameSession::AddPlayer(std::string dog_name) {
    try {
        Point start_pos = GenerateRandomPosition();
        Dog dog(std::move(dog_name), start_pos, Direction::North);

        static std::atomic<uint32_t> next_player_id_{0};
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        
        const char* hex_digits = "0123456789abcdef";
        std::string token;
        token.reserve(32);
        for (int i = 0; i < 32; ++i) {
            token += hex_digits[dis(gen)];
        }

        auto player = std::make_shared<Player>(
            shared_from_this(), 
            std::move(dog),
            next_player_id_++, 
            std::move(token)
        );

        players_.push_back(player);
        return player;
    } catch (const std::exception& e) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

void GameSession::SetPlayerAction(const Player::Token& token, const std::string& move) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& player : players_) {
        if (player->GetToken() != token) continue;
        
        auto& dog = player->GetDog();
        if (move == "L") {
            dog.SetSpeed({-dog_speed_, 0.0});
            dog.SetDirection(Direction::West);
        } else if (move == "R") {
            dog.SetSpeed({dog_speed_, 0.0});
            dog.SetDirection(Direction::East);
        } else if (move == "U") {
            dog.SetSpeed({0.0, -dog_speed_});
            dog.SetDirection(Direction::North);
        } else if (move == "D") {
            dog.SetSpeed({0.0, dog_speed_});
            dog.SetDirection(Direction::South);
        } else if (move == "") {
            dog.SetSpeed({0.0, 0.0});
        }
        break;
    }
}

void GameSession::GenerateLoot(std::chrono::milliseconds delta_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (loot_types_count_ == 0) return;

    unsigned looters_count = players_.size();
    unsigned current_loot_count = lost_objects_.size();
    
    unsigned new_loot_count = loot_generator_->Generate(delta_time, current_loot_count, looters_count);
    
    if (new_loot_count > 0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> type_dist(0, loot_types_count_ - 1);
        
        const auto& roads = map_.GetRoads();
        if (roads.empty()) return;
        
        std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
        
        for (unsigned i = 0; i < new_loot_count; ++i) {
            const auto& road = roads[road_dist(gen)];
            
            double x, y;
            if (road.IsHorizontal()) {
                std::uniform_real_distribution<double> x_dist(
                    std::min(road.GetStart().x, road.GetEnd().x),
                    std::max(road.GetStart().x, road.GetEnd().x)
                );
                x = x_dist(gen);
                y = road.GetStart().y;
            } else {
                x = road.GetStart().x;
                std::uniform_real_distribution<double> y_dist(
                    std::min(road.GetStart().y, road.GetEnd().y),
                    std::max(road.GetStart().y, road.GetEnd().y)
                );
                y = y_dist(gen);
            }
            
            lost_objects_.push_back({
                type_dist(gen),
                {x, y}
            });
        }
    }
}

/*void GameSession::Tick(double delta_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GenerateLoot(std::chrono::milliseconds(static_cast<int>(delta_time * 1000)));
    
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();

        if (speed.x == 0.0 && speed.y == 0.0) continue;

        double new_x = pos.x + speed.x * delta_time;
        double new_y = pos.y + speed.y * delta_time;
        
        // Проверяем, находится ли новая позиция на какой-либо дороге
        bool can_move = false;
        for (const auto& road : map_.GetRoads()) {
            auto bbox = road.GetBoundingBox();
            double road_x0 = bbox.position.x;
            double road_y0 = bbox.position.y;
            double road_x1 = road_x0 + bbox.size.width;
            double road_y1 = road_y0 + bbox.size.height;
            
            if (new_x >= road_x0 && new_x <= road_x1 &&
                new_y >= road_y0 && new_y <= road_y1) {
                can_move = true;
                break;
            }
        }
        
        if (can_move) {
            dog.SetPosition({new_x, new_y});
        } else {
            dog.SetSpeed({0.0, 0.0});
        }
    }
}*/



/*void GameSession::Tick(double delta_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GenerateLoot(std::chrono::milliseconds(static_cast<int>(delta_time * 1000)));
    
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();

        if (speed.x == 0.0 && speed.y == 0.0) continue;

        double new_x = pos.x + speed.x * delta_time;
        double new_y = pos.y + speed.y * delta_time;
        
        // Проверяем, можно ли двигаться в новую позицию
        bool can_move = false;
        
        // Сначала проверяем текущую позицию
        for (const auto& road : map_.GetRoads()) {
            auto bbox = road.GetBoundingBox();
            if (pos.x >= bbox.position.x && pos.x <= bbox.position.x + bbox.size.width &&
                pos.y >= bbox.position.y && pos.y <= bbox.position.y + bbox.size.height) {
                // Теперь проверяем новую позицию на ЭТОЙ ЖЕ дороге
                if (new_x >= bbox.position.x && new_x <= bbox.position.x + bbox.size.width &&
                    new_y >= bbox.position.y && new_y <= bbox.position.y + bbox.size.height) {
                    can_move = true;
                    break;
                }
            }
        }
        
        if (can_move) {
            dog.SetPosition({new_x, new_y});
        } else {
            dog.SetSpeed({0.0, 0.0});
        }
    }
}*/








void GameSession::Tick(double delta_time) {
    std::cout << "=== GameSession::Tick START ===" << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "DEBUG: Players count: " << players_.size() << std::endl;
    
    // Генерация лута
    GenerateLoot(std::chrono::milliseconds(static_cast<int>(delta_time * 1000)));
    
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();
        
        std::cout << "DEBUG: Player " << *player->GetId() 
                  << " - Pos: [" << pos.x << ", " << pos.y << "]"
                  << " - Speed: [" << speed.x << ", " << speed.y << "]" 
                  << " - Delta: " << delta_time << std::endl;

        if (speed.x == 0.0 && speed.y == 0.0) {
            std::cout << "DEBUG: Player " << *player->GetId() << " - No movement" << std::endl;
            continue;
        }

        // ПРОСТО ДВИГАЕМСЯ БЕЗ ПРОВЕРОК!
        double new_x = pos.x + speed.x * delta_time;
        double new_y = pos.y + speed.y * delta_time;
        
        std::cout << "DEBUG: Player " << *player->GetId() 
                  << " - New pos: [" << new_x << ", " << new_y << "]" << std::endl;
        
        dog.SetPosition({new_x, new_y});
    }
    
    std::cout << "=== GameSession::Tick END ===" << std::endl;
}









std::string DirectionToString(Direction dir) {
    switch (dir) {
        case Direction::North: return "U";
        case Direction::South: return "D";
        case Direction::West:  return "L";
        case Direction::East:  return "R";
    }
    return "U";
}

}  // namespace model
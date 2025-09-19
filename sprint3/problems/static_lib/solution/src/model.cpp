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

std::shared_ptr<Player> Game::JoinGame(const Map::Id& map_id, std::string dog_name) {
    const Map* map_ptr = nullptr;
    size_t loot_types_count = 0;
    
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);  // ИЗМЕНЕНО
        
        map_ptr = FindMap(map_id);
        if (!map_ptr) {
            return nullptr;
        }
        
        loot_types_count = GetMapLootTypesCount(map_id);
    }
    
    std::shared_ptr<GameSession> session;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);  // ИЗМЕНЕНО
        
        if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
            session = it->second;
        } else {
            double dog_speed = map_ptr->GetDogSpeed().value_or(default_dog_speed_);
            session = std::make_shared<GameSession>(*map_ptr, next_session_id_++, dog_speed,
                randomize_spawn_points_, loot_generator_);
            session->SetLootTypesCount(loot_types_count);
            map_id_to_session_[map_id] = session;
        }
    }
    
    auto player = session->AddPlayer(std::move(dog_name));
    if (!player) {
        return nullptr;
    }
    
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);  // ИЗМЕНЕНО
        token_to_player_[player->GetToken()] = player;
    }
    
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

        std::lock_guard<std::recursive_mutex> lock(mutex_);  // ИЗМЕНЕНО
        players_.push_back(player);
        return player;
    } catch (const std::exception& e) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

void GameSession::SetPlayerAction(const Player::Token& token, const std::string& move) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);  // ИЗМЕНЕНО
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
                next_lost_object_id_++,
                type_dist(gen),
                {x, y}
            });
        }
    }
}

void GameSession::Tick(double delta_time) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);  // ИЗМЕНЕНО
    
    GenerateLoot(std::chrono::milliseconds(static_cast<int>(delta_time * 1000)));
    
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();

        if (speed.x == 0.0 && speed.y == 0.0) continue;

        double new_x = pos.x + speed.x * delta_time;
        double new_y = pos.y + speed.y * delta_time;
        
        dog.SetPosition({new_x, new_y});
    }
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
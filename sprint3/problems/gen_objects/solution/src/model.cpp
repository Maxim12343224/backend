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
    std::cout << "JoinGame called for map: " << *map_id << std::endl;
    
    size_t loot_types_count = 0;
    const Map* map_ptr = nullptr;
    std::shared_ptr<GameSession> session;
    
    {
        std::cout << "Locking mutex for data access" << std::endl;
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "Mutex locked" << std::endl;
        
        map_ptr = FindMap(map_id);
        if (!map_ptr) {
            std::cout << "Map not found: " << *map_id << std::endl;
            return nullptr;
        }
        std::cout << "Map found: " << *map_id << std::endl;
        
        loot_types_count = GetMapLootTypesCount(map_id);
        std::cout << "Loot types count: " << loot_types_count << std::endl;
        
        if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
            session = it->second;
            std::cout << "Existing session found" << std::endl;
        } else {
            std::cout << "No existing session" << std::endl;
        }
    }
    std::cout << "Mutex unlocked" << std::endl;
    
    if (!session) {
        std::cout << "Creating new session" << std::endl;
        double dog_speed = map_ptr->GetDogSpeed().value_or(default_dog_speed_);
        session = std::make_shared<GameSession>(*map_ptr, next_session_id_++, dog_speed,
            randomize_spawn_points_, loot_generator_);
        session->SetLootTypesCount(loot_types_count);

        std::cout << "Locking mutex for session storage" << std::endl;
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "Mutex locked for session storage" << std::endl;
        
        if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
            session = it->second;
            std::cout << "Session already created by another thread" << std::endl;
        } else {
            map_id_to_session_[map_id] = session;
            std::cout << "New session stored" << std::endl;
        }
        std::cout << "Mutex unlocked after session storage" << std::endl;
    }

    std::cout << "Adding player" << std::endl;
    auto player = session->AddPlayer(std::move(dog_name));
    if (!player) {
        std::cout << "AddPlayer failed" << std::endl;
        return nullptr;
    }

    std::cout << "Locking mutex for token storage" << std::endl;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "Mutex locked for token storage" << std::endl;
        token_to_player_[player->GetToken()] = player;
        std::cout << "Mutex unlocked after token storage" << std::endl;
    }
    
    std::cout << "Player created successfully: " << *player->GetId() << std::endl;
    return player;
}*/









std::shared_ptr<Player> Game::JoinGame(const Map::Id& map_id, std::string dog_name) {
    std::cout << "JoinGame called for map: " << *map_id << std::endl;
    
    size_t loot_types_count = 0;
    const Map* map_ptr = nullptr;
    std::shared_ptr<GameSession> session;
    
    {
        // std::lock_guard<std::mutex> lock(mutex_);  // Закомментировано
        
        map_ptr = FindMap(map_id);
        if (!map_ptr) {
            std::cout << "Map not found: " << *map_id << std::endl;
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

        // std::lock_guard<std::mutex> lock(mutex_);  // Закомментировано
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
        // std::lock_guard<std::mutex> lock(mutex_);  // Закомментировано
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
        std::cout << "AddPlayer started" << std::endl;
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

        std::cout << "Creating player object" << std::endl;
        auto player = std::make_shared<Player>(
            shared_from_this(), 
            std::move(dog),
            next_player_id_++, 
            std::move(token)
        );

        players_.push_back(player);
        std::cout << "Player added successfully" << std::endl;
        return player;
    } catch (const std::exception& e) {
        std::cout << "Exception in AddPlayer: " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cout << "Unknown exception in AddPlayer" << std::endl;
        return nullptr;
    }
}

void GameSession::SetPlayerAction(const Player::Token& token, const std::string& move) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& player : players_) {
        if (player->GetToken() != token) continue;
        
        auto& dog = player->GetDog();
        Point new_speed{0.0, 0.0};
        if (move == "L") {
            new_speed = {-dog_speed_, 0.0};
            dog.SetDirection(Direction::West);
        } else if (move == "R") {
            new_speed = {dog_speed_, 0.0};
            dog.SetDirection(Direction::East);
        } else if (move == "U") {
            new_speed = {0.0, -dog_speed_};
            dog.SetDirection(Direction::North);
        } else if (move == "D") {
            new_speed = {0.0, dog_speed_};
            dog.SetDirection(Direction::South);
        } else if (move == "") {
            new_speed = {0.0, 0.0};
        }
        dog.SetSpeed(new_speed);
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

void GameSession::Tick(double delta_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GenerateLoot(std::chrono::milliseconds(static_cast<int>(delta_time * 1000)));
    
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();

        if (speed.x == 0.0 && speed.y == 0.0) continue;

        const double max_step = 0.1;
        double remaining_time = delta_time;
        
        while (remaining_time > 0.0) {
            double step_time = std::min(remaining_time, max_step);
            remaining_time -= step_time;

            double move_x = speed.x * step_time;
            double move_y = speed.y * step_time;
            double new_x = pos.x + move_x;
            double new_y = pos.y + move_y;

            bool can_move = false;
            for (const auto& road : map_.GetRoads()) {
                auto bbox = road.GetBoundingBox();
                double road_x0 = bbox.position.x;
                double road_y0 = bbox.position.y;
                double road_x1 = road_x0 + bbox.size.width;
                double road_y1 = road_y0 + bbox.size.height;

                if (pos.x >= road_x0 && pos.x <= road_x1 &&
                    pos.y >= road_y0 && pos.y <= road_y1) {
                    
                    if (new_x >= road_x0 && new_x <= road_x1 &&
                        new_y >= road_y0 && new_y <= road_y1) {
                        can_move = true;
                        break;
                    }
                }
            }

            if (can_move) {
                pos.x = new_x;
                pos.y = new_y;
                dog.SetPosition(pos);
            } else {
                double target_x = new_x;
                double target_y = new_y;
                bool hit_boundary = false;
                
                for (const auto& road : map_.GetRoads()) {
                    auto bbox = road.GetBoundingBox();
                    double road_x0 = bbox.position.x;
                    double road_y0 = bbox.position.y;
                    double road_x1 = road_x0 + bbox.size.width;
                    double road_y1 = road_y0 + bbox.size.height;

                    if (pos.x >= road_x0 && pos.x <= road_x1 &&
                        pos.y >= road_y0 && pos.y <= road_y1) {
                        
                        if (speed.x != 0) {
                            if (speed.x > 0) {
                                target_x = std::min(new_x, road_x1);
                            } else {
                                target_x = std::max(new_x, road_x0);
                            }
                            hit_boundary = true;
                        }
                        
                        if (speed.y != 0) {
                            if (speed.y > 0) {
                                target_y = std::min(new_y, road_y1);
                            } else {
                                target_y = std::max(new_y, road_y0);
                            }
                            hit_boundary = true;
                        }
                        
                        if (hit_boundary) break;
                    }
                }
                
                pos.x = target_x;
                pos.y = target_y;
                dog.SetPosition(pos);
                dog.SetSpeed({0.0, 0.0});
                break;
            }
        }
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
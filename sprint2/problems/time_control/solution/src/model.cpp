#include "model.h"
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cmath>

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

Point GameSession::GenerateStartPosition() const {
    const auto& roads = map_.GetRoads();
    if (roads.empty()) {
        return {0.0, 0.0};
    }
    return {static_cast<double>(roads.front().GetStart().x),
            static_cast<double>(roads.front().GetStart().y)};
}

std::shared_ptr<Player> GameSession::AddPlayer(std::string dog_name) {
    Point start_pos = GenerateStartPosition();
    Dog dog(std::move(dog_name), start_pos, Direction::North);
    dog.SetCurrentRoad(&map_.GetRoads().front());

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
}

void GameSession::SetPlayerAction(const Player::Token& token, const std::string& move) {
    std::lock_guard lock(mutex_);
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

void GameSession::Tick(double delta_time) {
    std::lock_guard lock(mutex_);
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto* road = dog.GetCurrentRoad();
        if (!road) {
            continue;
        }

        Point new_pos = dog.GetPosition();
        Point speed = dog.GetSpeed();

        if (speed.x == 0 && speed.y == 0) {
            continue;
        }

        // Вычисляем новую позицию без ограничений
        new_pos.x += speed.x * delta_time;
        new_pos.y += speed.y * delta_time;

        // Проверяем, находится ли новая позиция в пределах дороги
        auto is_valid_position = [road](const Point& pos) {
            if (road->IsHorizontal()) {
                const double min_x = std::min(road->GetStart().x, road->GetEnd().x) - 0.4;
                const double max_x = std::max(road->GetStart().x, road->GetEnd().x) + 0.4;
                const double road_y = road->GetStart().y;
                return (pos.x >= min_x && pos.x <= max_x && 
                        std::abs(pos.y - road_y) <= 0.4);
            } else {
                const double min_y = std::min(road->GetStart().y, road->GetEnd().y) - 0.4;
                const double max_y = std::max(road->GetStart().y, road->GetEnd().y) + 0.4;
                const double road_x = road->GetStart().x;
                return (pos.y >= min_y && pos.y <= max_y && 
                        std::abs(pos.x - road_x) <= 0.4);
            }
        };

        // Если позиция невалидна, корректируем её
        if (!is_valid_position(new_pos)) {
            if (road->IsHorizontal()) {
                // Корректируем X координату
                double min_x = std::min(road->GetStart().x, road->GetEnd().x) - 0.4;
                double max_x = std::max(road->GetStart().x, road->GetEnd().x) + 0.4;
                new_pos.x = std::clamp(new_pos.x, min_x, max_x);
                
                // Корректируем Y координату
                new_pos.y = road->GetStart().y;
            } else {
                // Корректируем Y координату
                double min_y = std::min(road->GetStart().y, road->GetEnd().y) - 0.4;
                double max_y = std::max(road->GetStart().y, road->GetEnd().y) + 0.4;
                new_pos.y = std::clamp(new_pos.y, min_y, max_y);
                
                // Корректируем X координату
                new_pos.x = road->GetStart().x;
            }
            
            // Обнуляем скорость при столкновении с границей
            speed = {0.0, 0.0};
        }

        dog.SetPosition(new_pos);
        dog.SetSpeed(speed);
    }
}

void Game::Tick(double delta_time) {
    for (auto& [map_id, session] : map_id_to_session_) {
        session->Tick(delta_time);
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
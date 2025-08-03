#include "model.h"
#include <stdexcept>
#include <random>
#include <algorithm>

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

bool Map::IsPointOnRoads(Point p) const {
    for (const auto& road : roads_) {
        if (road.IsHorizontal()) {
            double road_y = road.GetStart().y;
            double x0 = std::min(road.GetStart().x, road.GetEnd().x);
            double x1 = std::max(road.GetStart().x, road.GetEnd().x);
            
            // Расширяем допустимую зону до границ дороги
            if (p.y >= road_y - 0.4 - 1e-5 && 
                p.y <= road_y + 0.4 + 1e-5 &&
                p.x >= x0 - 1e-5 && 
                p.x <= x1 + 1e-5) {
                return true;
            }
        } else if (road.IsVertical()) {
            double road_x = road.GetStart().x;
            double y0 = std::min(road.GetStart().y, road.GetEnd().y);
            double y1 = std::max(road.GetStart().y, road.GetEnd().y);
            
            // Расширяем допустимую зону до границ дороги
            if (p.x >= road_x - 0.4 - 1e-5 && 
                p.x <= road_x + 0.4 + 1e-5 &&
                p.y >= y0 - 1e-5 && 
                p.y <= y1 + 1e-5) {
                return true;
            }
        }
    }
    return false;
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

Point GameSession::GenerateRandomPosition() const {
    const auto& roads = map_.GetRoads();
    if (roads.empty()) {
        return {0.0, 0.0};
    }
    
    // Начальная точка первой дороги с учетом смещения
    const auto& road = roads.front();
    Point start = road.GetStart();
    
    // Для горизонтальных дорог добавляем смещение
    if (road.IsHorizontal()) {
        return {static_cast<double>(start.x), static_cast<double>(start.y)};
    }
    // Для вертикальных дорог добавляем смещение
    return {static_cast<double>(start.x), static_cast<double>(start.y)};
}

std::shared_ptr<Player> GameSession::AddPlayer(std::string dog_name) {
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

void GameSession::UpdateState(double delta_time_sec) {
    std::lock_guard lock(mutex_);
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        Point speed = dog.GetSpeed();
        if (speed.x == 0.0 && speed.y == 0.0) {
            continue;
        }

        Point new_position = dog.GetPosition();
        new_position.x += speed.x * delta_time_sec;
        new_position.y += speed.y * delta_time_sec;

        // Рассчитываем конечную позицию с учетом дорожных ограничений
        Point final_position = new_position;
        bool moved = false;
        
        for (const auto& road : map_.GetRoads()) {
            if (road.IsHorizontal()) {
                double road_y = road.GetStart().y;
                double x0 = std::min(road.GetStart().x, road.GetEnd().x);
                double x1 = std::max(road.GetStart().x, road.GetEnd().x);
                
                // Проверяем, движется ли собака вдоль горизонтальной дороги
                if (std::abs(new_position.y - road_y) <= 0.4 + 1e-5) {
                    // Ограничиваем движение по X
                    final_position.x = std::clamp(new_position.x, x0, x1);
                    final_position.y = road_y;  // Фиксируем позицию по Y
                    moved = true;
                    break;
                }
            } else if (road.IsVertical()) {
                double road_x = road.GetStart().x;
                double y0 = std::min(road.GetStart().y, road.GetEnd().y);
                double y1 = std::max(road.GetStart().y, road.GetEnd().y);
                
                // Проверяем, движется ли собака вдоль вертикальной дороги
                if (std::abs(new_position.x - road_x) <= 0.4 + 1e-5) {
                    // Ограничиваем движение по Y
                    final_position.y = std::clamp(new_position.y, y0, y1);
                    final_position.x = road_x;  // Фиксируем позицию по X
                    moved = true;
                    break;
                }
            }
        }

        // Если перемещение допустимо, обновляем позицию
        if (moved) {
            dog.SetPosition(final_position);
            
            // Сбрасываем скорость только если достигли границы
            if (final_position.x != new_position.x || final_position.y != new_position.y) {
                dog.SetSpeed({0.0, 0.0});
            }
        } else {
            // Если точка не на дороге, сбрасываем скорость
            dog.SetSpeed({0.0, 0.0});
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
#include "model.h"
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cmath>
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






Point GameSession::GenerateRandomPosition() const {
    const auto& roads = map_.GetRoads();
    if (roads.empty()) return {0.0, 0.0};
    
    // Появление строго в начальной точке первой дороги
    const auto& road = roads[0];
    return {
        static_cast<double>(road.GetStart().x),
        static_cast<double>(road.GetStart().y)
    };
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

void GameSession::Tick(double delta_time) {
    std::lock_guard lock(mutex_);
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        const auto pos = dog.GetPosition();
        const auto speed = dog.GetSpeed();

        // Если собака не движется, пропускаем обработку
        if (speed.x == 0.0 && speed.y == 0.0) continue;

        // Рассчитываем вектор перемещения
        const double move_x = speed.x * delta_time;
        const double move_y = speed.y * delta_time;
        const double new_x = pos.x + move_x;
        const double new_y = pos.y + move_y;

        // Проверяем, принадлежит ли новая позиция любой дороге на карте
        auto is_valid_position = [this](double x, double y) {
            for (const auto& road : map_.GetRoads()) {
                const auto bbox = road.GetBoundingBox();
                const double road_x0 = bbox.position.x;
                const double road_y0 = bbox.position.y;
                const double road_x1 = road_x0 + bbox.size.width;
                const double road_y1 = road_y0 + bbox.size.height;
                
                if (x >= road_x0 && x <= road_x1 &&
                    y >= road_y0 && y <= road_y1) {
                    return true;
                }
            }
            return false;
        };

        // Если новое положение валидно - перемещаем
        if (is_valid_position(new_x, new_y)) {
            dog.SetPosition({new_x, new_y});
        } else {
            // Определяем текущую дорогу
            const Road* current_road = nullptr;
            for (const auto& road : map_.GetRoads()) {
                const auto bbox = road.GetBoundingBox();
                const double road_x0 = bbox.position.x;
                const double road_y0 = bbox.position.y;
                const double road_x1 = road_x0 + bbox.size.width;
                const double road_y1 = road_y0 + bbox.size.height;
                
                if (pos.x >= road_x0 && pos.x <= road_x1 &&
                    pos.y >= road_y0 && pos.y <= road_y1) {
                    current_road = &road;
                    break;
                }
            }

            // Если текущая дорога найдена, обрабатываем границы
            if (current_road) {
                const auto bbox = current_road->GetBoundingBox();
                const double road_x0 = bbox.position.x;
                const double road_y0 = bbox.position.y;
                const double road_x1 = road_x0 + bbox.size.width;
                const double road_y1 = road_y0 + bbox.size.height;

                // Вычисляем целевую позицию с учетом границ дороги
                double target_x = new_x;
                double target_y = new_y;
                bool boundary_hit = false;

                // Проверка горизонтальных границ
                if (speed.x != 0.0) {
                    if (speed.x > 0 && new_x > road_x1) {
                        target_x = road_x1;
                        boundary_hit = true;
                    } else if (speed.x < 0 && new_x < road_x0) {
                        target_x = road_x0;
                        boundary_hit = true;
                    }
                }

                // Проверка вертикальных границ
                if (speed.y != 0.0) {
                    if (speed.y > 0 && new_y > road_y1) {
                        target_y = road_y1;
                        boundary_hit = true;
                    } else if (speed.y < 0 && new_y < road_y0) {
                        target_y = road_y0;
                        boundary_hit = true;
                    }
                }

                // Если уперлись в границу - устанавливаем позицию и останавливаем движение
                if (boundary_hit) {
                    // Проверяем, не ведет ли граница на другую дорогу
                    if (is_valid_position(target_x, target_y)) {
                        dog.SetPosition({target_x, target_y});
                    } else {
                        // Если за границей нет дороги - останавливаем на границе
                        dog.SetPosition({target_x, target_y});
                        dog.SetSpeed({0.0, 0.0});
                    }
                } else {
                    // Если не уперлись в границу, но позиция невалидна - возможно, диагональное движение
                    // Пробуем переместиться только по одной оси
                    if (is_valid_position(new_x, pos.y)) {
                        dog.SetPosition({new_x, pos.y});
                    } else if (is_valid_position(pos.x, new_y)) {
                        dog.SetPosition({pos.x, new_y});
                    } else {
                        // Если ни один вариант не работает - останавливаем
                        dog.SetSpeed({0.0, 0.0});
                    }
                }
            } else {
                // Если собака не на дороге - останавливаем
                dog.SetSpeed({0.0, 0.0});
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
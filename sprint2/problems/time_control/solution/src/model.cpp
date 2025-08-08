#include <iostream>
#include "model.h"
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iomanip>



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


        std::cerr << "Setting action: " << move << " for token: " << *token << std::endl;



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



        std::cerr << "New speed set: (" << new_speed.x << ", " << new_speed.y << ")" << std::endl;




        break;
    }
}

/*void GameSession::Tick(double delta_time) {
    std::lock_guard lock(mutex_);
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();

        if (speed.x == 0.0 && speed.y == 0.0) continue;

        double move_x = speed.x * delta_time;
        double move_y = speed.y * delta_time;
        double new_x = pos.x + move_x;
        double new_y = pos.y + move_y;

        // Проверяем каждую дорогу на возможность движения
        bool can_move = false;
        for (const auto& road : map_.GetRoads()) {
            auto bbox = road.GetBoundingBox();
            double road_x0 = bbox.position.x;
            double road_y0 = bbox.position.y;
            double road_x1 = road_x0 + bbox.size.width;
            double road_y1 = road_y0 + bbox.size.height;

            // Текущая позиция на дороге?
            if (pos.x >= road_x0 && pos.x <= road_x1 &&
                pos.y >= road_y0 && pos.y <= road_y1) {
                
                // Проверка новой позиции
                if (new_x >= road_x0 && new_x <= road_x1 &&
                    new_y >= road_y0 && new_y <= road_y1) {
                    can_move = true;
                    break;
                }
            }
        }

        if (can_move) {
            dog.SetPosition({new_x, new_y});
        } else {
            // Вычисляем ближайшую границу
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
                    
                    // Горизонтальное движение
                    if (speed.x != 0) {
                        if (speed.x > 0) {
                            target_x = std::min(new_x, road_x1);
                        } else {
                            target_x = std::max(new_x, road_x0);
                        }
                        hit_boundary = true;
                    }
                    
                    // Вертикальное движение
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
            
            dog.SetPosition({target_x, target_y});
            dog.SetSpeed({0.0, 0.0});
        }
    }
}*/




void GameSession::Tick(double delta_time) {
    std::lock_guard lock(mutex_);
    for (auto& player : players_) {
        auto& dog = player->GetDog();
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();

        if (speed.x == 0.0 && speed.y == 0.0) continue;

        double new_x = pos.x + speed.x * delta_time;
        double new_y = pos.y + speed.y * delta_time;

        // 1. Проверяем, разрешено ли движение на новой позиции на ЛЮБОЙ дороге карты
        bool can_move = false;
        for (const auto& road : map_.GetRoads()) {
            auto bbox = road.GetBoundingBox();
            if (new_x >= bbox.position.x && 
                new_x <= bbox.position.x + bbox.size.width &&
                new_y >= bbox.position.y && 
                new_y <= bbox.position.y + bbox.size.height) {
                can_move = true;
                break;
            }
        }

        if (can_move) {
            // Движение разрешено - обновляем позицию
            dog.SetPosition({new_x, new_y});
        } else {
            // 2. Корректируем позицию до ближайшей границы
            double target_x = new_x;
            double target_y = new_y;
            bool corrected = false;

            // Для горизонтального движения
            if (speed.x != 0.0) {
                if (speed.x > 0) {
                    // Движение вправо - ищем минимальную правую границу
                    double min_right = std::numeric_limits<double>::max();
                    for (const auto& road : map_.GetRoads()) {
                        auto bbox = road.GetBoundingBox();
                        // Проверяем только дороги, на которых мы сейчас находимся
                        if (pos.x >= bbox.position.x && pos.x <= bbox.position.x + bbox.size.width &&
                            pos.y >= bbox.position.y && pos.y <= bbox.position.y + bbox.size.height) {
                            double road_right = bbox.position.x + bbox.size.width;
                            if (road_right < min_right) {
                                min_right = road_right;
                                corrected = true;
                            }
                        }
                    }
                    if (corrected) target_x = min_right;
                } else {
                    // Движение влево - ищем максимальную левую границу
                    double max_left = std::numeric_limits<double>::lowest();
                    for (const auto& road : map_.GetRoads()) {
                        auto bbox = road.GetBoundingBox();
                        if (pos.x >= bbox.position.x && pos.x <= bbox.position.x + bbox.size.width &&
                            pos.y >= bbox.position.y && pos.y <= bbox.position.y + bbox.size.height) {
                            double road_left = bbox.position.x;
                            if (road_left > max_left) {
                                max_left = road_left;
                                corrected = true;
                            }
                        }
                    }
                    if (corrected) target_x = max_left;
                }
            }

            // Для вертикального движения
            if (speed.y != 0.0) {
                if (speed.y > 0) {
                    // Движение вниз - ищем минимальную нижнюю границу
                    double min_bottom = std::numeric_limits<double>::max();
                    for (const auto& road : map_.GetRoads()) {
                        auto bbox = road.GetBoundingBox();
                        if (pos.x >= bbox.position.x && pos.x <= bbox.position.x + bbox.size.width &&
                            pos.y >= bbox.position.y && pos.y <= bbox.position.y + bbox.size.height) {
                            double road_bottom = bbox.position.y + bbox.size.height;
                            if (road_bottom < min_bottom) {
                                min_bottom = road_bottom;
                                corrected = true;
                            }
                        }
                    }
                    if (corrected) target_y = min_bottom;
                } else {
                    // Движение вверх - ищем максимальную верхнюю границу
                    double max_top = std::numeric_limits<double>::lowest();
                    for (const auto& road : map_.GetRoads()) {
                        auto bbox = road.GetBoundingBox();
                        if (pos.x >= bbox.position.x && pos.x <= bbox.position.x + bbox.size.width &&
                            pos.y >= bbox.position.y && pos.y <= bbox.position.y + bbox.size.height) {
                            double road_top = bbox.position.y;
                            if (road_top > max_top) {
                                max_top = road_top;
                                corrected = true;
                            }
                        }
                    }
                    if (corrected) target_y = max_top;
                }
            }

            // Устанавливаем скорректированную позицию
            dog.SetPosition({target_x, target_y});
            
            // Останавливаем собаку только если была корректировка
            if (corrected) {
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
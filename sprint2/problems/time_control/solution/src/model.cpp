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
    if (roads.empty()) {
        return {0.0, 0.0};
    }

    // Появление в начальной точке первой дороги
    const auto& road = roads[0];
    return { static_cast<double>(road.GetStart().x), static_cast<double>(road.GetStart().y) };
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
        auto pos = dog.GetPosition();
        auto speed = dog.GetSpeed();

        if (speed.x == 0.0 && speed.y == 0.0) {
            continue;
        }

        double new_x = pos.x + speed.x * delta_time;
        double new_y = pos.y + speed.y * delta_time;

        // Проверка дорог для определения границ
        bool can_move = true;
        Point new_pos = {new_x, new_y};

        for (const auto& road : map_.GetRoads()) {
            auto bbox = road.GetBoundingBox();
            double x0 = bbox.position.x;
            double y0 = bbox.position.y;
            double x1 = x0 + bbox.size.width;
            double y1 = y0 + bbox.size.height;

            // Проверка находится ли текущая позиция в пределах дороги
            bool on_road = (pos.x >= x0 && pos.x <= x1 && pos.y >= y0 && pos.y <= y1);
            if (!on_road) continue;

            // Проверка новой позиции
            bool x_in_road = (new_x >= x0 && new_x <= x1);
            bool y_in_road = (new_y >= y0 && new_y <= y1);
            if (!x_in_road || !y_in_road) {
                can_move = false;
                break;
            }
        }

        if (can_move) {
            dog.SetPosition(new_pos);
        } else {
            // Остановка собаки, если движение заблокировано
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
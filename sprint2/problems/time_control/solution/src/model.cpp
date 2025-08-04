#include "model.h"
#include <stdexcept>
#include <random>
#include <cmath>
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

namespace {
    bool IsPointOnRoad(const Road& road, Point p) {
        constexpr double tolerance = 0.4;
        if (road.IsHorizontal()) {
            auto min_x = std::min(road.GetStart().x, road.GetEnd().x);
            auto max_x = std::max(road.GetStart().x, road.GetEnd().x);
            return p.x >= min_x && p.x <= max_x && 
                   std::abs(p.y - road.GetStart().y) <= tolerance;
        } else {
            auto min_y = std::min(road.GetStart().y, road.GetEnd().y);
            auto max_y = std::max(road.GetStart().y, road.GetEnd().y);
            return p.y >= min_y && p.y <= max_y && 
                   std::abs(p.x - road.GetStart().x) <= tolerance;
        }
    }

    const Road* FindHorizontalRoad(Point p, const Map& map) {
        for (const auto& road : map.GetRoads()) {
            if (!road.IsHorizontal()) continue;
            if (IsPointOnRoad(road, p)) {
                return &road;
            }
        }
        return nullptr;
    }

    const Road* FindVerticalRoad(Point p, const Map& map) {
        for (const auto& road : map.GetRoads()) {
            if (!road.IsVertical()) continue;
            if (IsPointOnRoad(road, p)) {
                return &road;
            }
        }
        return nullptr;
    }
}

void Dog::UpdatePosition(double delta_time, const Map& map) {
    double new_x = position_.x + speed_.x * delta_time;
    double new_y = position_.y + speed_.y * delta_time;

    // Horizontal movement
    if (speed_.x != 0) {
        const auto* h_road = FindHorizontalRoad(position_, map);
        if (h_road) {
            double min_x = std::min(h_road->GetStart().x, h_road->GetEnd().x);
            double max_x = std::max(h_road->GetStart().x, h_road->GetEnd().x);
            new_x = std::clamp(new_x, min_x, max_x);
            if (new_x <= min_x || new_x >= max_x) {
                speed_.x = 0;
            }
        } else {
            new_x = position_.x;
            speed_.x = 0;
        }
    }

    // Vertical movement
    if (speed_.y != 0) {
        Point temp_pos{new_x, position_.y};
        const auto* v_road = FindVerticalRoad(temp_pos, map);
        if (v_road) {
            double min_y = std::min(v_road->GetStart().y, v_road->GetEnd().y);
            double max_y = std::max(v_road->GetStart().y, v_road->GetEnd().y);
            new_y = std::clamp(new_y, min_y, max_y);
            if (new_y <= min_y || new_y >= max_y) {
                speed_.y = 0;
            }
        } else {
            new_y = position_.y;
            speed_.y = 0;
        }
    }

    position_ = {new_x, new_y};
}

std::shared_ptr<Player> GameSession::AddPlayer(std::string dog_name) {
    Point start_pos;
    if (map_.GetRoads().empty()) {
        start_pos = {0.0, 0.0};
    } else {
        const auto& first_road = map_.GetRoads()[0];
        start_pos = first_road.GetStart();
    }

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

void GameSession::UpdateState(double delta_time) {
    std::lock_guard lock(mutex_);
    for (auto& player : players_) {
        player->GetDog().UpdatePosition(delta_time, map_);
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
#include "model.h"
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <variant>

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
    size_t bag_capacity = 0;
    std::vector<int> loot_values;
    
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        map_ptr = FindMap(map_id);
        if (!map_ptr) {
            return nullptr;
        }
        
        loot_types_count = GetMapLootTypesCount(map_id);
        bag_capacity = GetMapBagCapacity(map_id);
        loot_values = GetMapLootValues(map_id);
    }
    
    std::shared_ptr<GameSession> session;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
            session = it->second;
        } else {
            double dog_speed = map_ptr->GetDogSpeed().value_or(default_dog_speed_);
            session = std::make_shared<GameSession>(*map_ptr, next_session_id_++, dog_speed,
                randomize_spawn_points_, loot_generator_, bag_capacity, retirement_time_);
            session->SetLootTypesCount(loot_types_count);
            session->SetLootValues(loot_values);
            map_id_to_session_[map_id] = session;
        }
    }
    
    auto player = session->AddPlayer(std::move(dog_name));
    if (!player) {
        return nullptr;
    }
    
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
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
            std::move(token),
            bag_capacity_
        );

        std::lock_guard<std::recursive_mutex> lock(mutex_);
        players_.push_back(player);
        return player;
    } catch (const std::exception& e) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

void GameSession::SetPlayerAction(const Player::Token& token, const std::string& move) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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

    unsigned looters_count = 0;
    for (const auto& player : players_) {
        if (!player->IsRetired()) {
            looters_count++;
        }
    }
    
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
            
            size_t type = type_dist(gen);
            int value = 0;
            if (type < loot_values_.size()) {
                value = loot_values_[type];
            }
            
            lost_objects_.push_back({
                next_lost_object_id_++,
                type,
                {x, y},
                value
            });
        }
    }
}

std::vector<std::shared_ptr<Player>> GameSession::GetActivePlayers() const {
    std::vector<std::shared_ptr<Player>> active_players;
    for (const auto& player : players_) {
        if (!player->IsRetired()) {
            active_players.push_back(player);
        }
    }
    return active_players;
}

std::vector<std::shared_ptr<Player>> GameSession::CheckRetiredPlayers() {
    std::vector<std::shared_ptr<Player>> retired_players;
    auto now = std::chrono::steady_clock::now();
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto& player : players_) {
        if (player->IsRetired()) continue;
        
        const auto& dog = player->GetDog();
        auto last_move = dog.GetLastMoveTime();
        auto time_since_last_move = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_move);
        
        if (time_since_last_move >= retirement_time_) {
            player->Retire();
            retired_players.push_back(player);
        }
    }
    
    return retired_players;
}

namespace {

struct ItemGathererProviderImpl : public collision_detector::ItemGathererProvider {
    struct ItemInfo {
        geom::Point2D position;
        double width;
        size_t id;
    };

    struct GathererInfo {
        geom::Point2D start_pos;
        geom::Point2D end_pos;
        double width;
        size_t id;
        std::shared_ptr<Player> player;
    };

    std::vector<ItemInfo> items;
    std::vector<GathererInfo> gatherers;

    size_t ItemsCount() const override { return items.size(); }
    collision_detector::Item GetItem(size_t idx) const override {
        const auto& item = items[idx];
        return {item.position, item.width};
    }
    size_t GatherersCount() const override { return gatherers.size(); }
    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        const auto& gatherer = gatherers[idx];
        return {gatherer.start_pos, gatherer.end_pos, gatherer.width};
    }
};

struct Event {
    enum Type { GATHER, RETURN } type;
    double time;
    size_t gatherer_id;
    size_t item_id;
    double sq_distance;
};

}  // namespace

void GameSession::Tick(double delta_time) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Убираем ушедших игроков из активных
    auto active_players = GetActivePlayers();
    
    std::vector<Point> start_positions;
    std::vector<Point> end_positions;
    
    for (const auto& player : active_players) {
        const auto& dog = player->GetDog();
        start_positions.push_back(dog.GetPosition());
        
        Point end_pos = {
            dog.GetPosition().x + dog.GetSpeed().x * delta_time,
            dog.GetPosition().y + dog.GetSpeed().y * delta_time
        };
        end_positions.push_back(end_pos);
    }
    
    GenerateLoot(std::chrono::milliseconds(static_cast<int>(delta_time * 1000)));
    
    for (size_t i = 0; i < active_players.size(); ++i) {
        auto& dog = active_players[i]->GetDog();
        dog.SetPosition(end_positions[i]);
    }
    
    ItemGathererProviderImpl provider;
    
    for (const auto& obj : lost_objects_) {
        provider.items.push_back({
            {obj.position.x, obj.position.y},
            0.0,
            obj.id
        });
    }
    
    for (size_t i = 0; i < active_players.size(); ++i) {
        provider.gatherers.push_back({
            {start_positions[i].x, start_positions[i].y},
            {end_positions[i].x, end_positions[i].y},
            0.3,
            i,
            active_players[i]
        });
    }
    
    const auto& offices = map_.GetOffices();
    for (const auto& office : offices) {
        provider.items.push_back({
            {office.GetPosition().x, office.GetPosition().y},
            0.25,
            std::numeric_limits<size_t>::max()
        });
    }
    
    auto gather_events = collision_detector::FindGatherEvents(provider);
    
    std::vector<Event> events;
    for (const auto& e : gather_events) {
        bool is_office = provider.items[e.item_id].id == std::numeric_limits<size_t>::max();
        
        events.push_back({
            is_office ? Event::RETURN : Event::GATHER,
            e.time,
            e.gatherer_id,
            e.item_id,
            e.sq_distance
        });
    }
    
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        return a.time < b.time;
    });
    
    std::vector<LostObject> collected_items;
    std::vector<bool> item_processed(lost_objects_.size(), false);
    
    for (const auto& event : events) {
        auto& gatherer = provider.gatherers[event.gatherer_id];
        auto& player = gatherer.player;
        
        if (player->IsRetired()) continue;
        
        if (event.type == Event::GATHER) {
            size_t item_idx = event.item_id;
            if (item_idx >= lost_objects_.size()) continue;
            
            if (!item_processed[item_idx] && !player->IsBagFull()) {
                if (player->AddItemToBag(lost_objects_[item_idx])) {
                    item_processed[item_idx] = true;
                    collected_items.push_back(lost_objects_[item_idx]);
                }
            }
        } else if (event.type == Event::RETURN) {
            for (const auto& item : player->GetBag()) {
                player->AddScore(item.value);
            }
            player->ClearBag();
        }
    }
    
    auto new_end = std::remove_if(lost_objects_.begin(), lost_objects_.end(),
        [&](const LostObject& obj) {
            auto it = std::find_if(collected_items.begin(), collected_items.end(),
                [&](const LostObject& collected) { return collected.id == obj.id; });
            return it != collected_items.end();
        });
    lost_objects_.erase(new_end, lost_objects_.end());
}

// Реализации методов для сериализации/десериализации
void GameSession::AddRestoredPlayer(std::shared_ptr<Player> player) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    players_.push_back(player);
}

void GameSession::AddRestoredLostObject(const LostObject& obj) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    lost_objects_.push_back(obj);
}

void GameSession::SetNextLostObjectId(size_t id) {
    next_lost_object_id_.store(id);
}

size_t GameSession::GetNextLostObjectId() const {
    return next_lost_object_id_.load();
}

std::vector<std::shared_ptr<Player>> Game::GetAllPlayers() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::shared_ptr<Player>> players;
    for (const auto& [token, player] : token_to_player_) {
        players.push_back(player);
    }
    return players;
}

void Game::AddRestoredSession(const Map::Id& map_id, std::shared_ptr<GameSession> session) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    map_id_to_session_[map_id] = session;
    
    // Восстанавливаем mapping токенов для игроков этой сессии
    for (const auto& player : session->GetPlayers()) {
        token_to_player_[player->GetToken()] = player;
    }
    
    uint32_t session_id = *session->GetId();
    if (session_id >= next_session_id_.load()) {
        next_session_id_.store(session_id + 1);
    }
}

void Game::RestoreTokenToPlayerMapping(const Player::Token& token, std::shared_ptr<Player> player) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    token_to_player_[token] = player;
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
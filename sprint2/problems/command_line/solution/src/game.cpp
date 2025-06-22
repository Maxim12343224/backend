#include "game.h"
#include <algorithm>
#include <stdexcept>
#include <random>

namespace model {

    Game::Game(double default_dog_speed)
        : default_dog_speed_(default_dog_speed),
        randomize_spawn_points_(false),
        generator_(random_device_()) {}

    void Game::SetDefaultDogSpeed(double speed) noexcept {
        default_dog_speed_ = speed;
    }

    double Game::GetDefaultDogSpeed() const noexcept {
        return default_dog_speed_;
    }

    const Game::Maps& Game::GetMaps() const noexcept {
        return maps_;
    }

    const Map* Game::FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    void Game::AddMap(Map map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id " + *map.GetId() + " already exists");
        }
        else {
            try {
                maps_.emplace_back(std::move(map));
            }
            catch (...) {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    void Game::SetRandomSpawnPoints(bool randomize) noexcept {
        randomize_spawn_points_ = randomize;
    }

    bool Game::IsRandomSpawnPoints() const noexcept {
        return randomize_spawn_points_;
    }

    std::shared_ptr<Player> Game::JoinGame(const std::string& player_name, const Map::Id& map_id) {
        if (const auto* map = FindMap(map_id)) {
            Point spawn_point;

            if (randomize_spawn_points_) {
                if (!map->GetRoads().empty()) {
                    const auto& roads = map->GetRoads();
                    std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
                    const auto& road = roads[road_dist(generator_)];

                    if (road.IsHorizontal()) {
                        int x_start = std::min(road.GetStart().x, road.GetEnd().x);
                        int x_end = std::max(road.GetStart().x, road.GetEnd().x);
                        std::uniform_int_distribution<int> x_dist(x_start, x_end);
                        spawn_point = { x_dist(generator_), road.GetStart().y };
                    }
                    else {
                        int y_start = std::min(road.GetStart().y, road.GetEnd().y);
                        int y_end = std::max(road.GetStart().y, road.GetEnd().y);
                        std::uniform_int_distribution<int> y_dist(y_start, y_end);
                        spawn_point = { road.GetStart().x, y_dist(generator_) };
                    }
                }
                else {
                    spawn_point = { 0, 0 };
                }
            }
            else {
                spawn_point = map->GetSpawnPoint();
            }

            auto dog = std::make_shared<Dog>(Dog::Id{ "" }, player_name, spawn_point);
            dog->SetMap(map);
            auto player = players_.Add(player_name, dog);
            player->SetToken(token_generator_.GenerateToken());
            return player;
        }
        return nullptr;
    }

    const std::vector<std::shared_ptr<Player>>& Game::GetPlayers() const {
        return players_.GetPlayers();
    }

    std::shared_ptr<Player> Game::FindPlayerByToken(const Token& token) {
        return players_.FindByToken(token);
    }

    void Game::UpdateState(int delta_time) {
        for (const auto& player : players_.GetPlayers()) {
            player->GetDog().UpdatePosition(delta_time);
        }
    }

} // namespace model
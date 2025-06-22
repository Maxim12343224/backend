#pragma once
#include "map.h"
#include "player.h"
#include "token_generator.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace model {

    class Game {
    public:
        using Maps = std::vector<Map>;

        explicit Game(double default_dog_speed = 1.0);
        ~Game() = default;

        // «апрещаем копирование и перемещение
        Game(const Game&) = delete;
        Game& operator=(const Game&) = delete;
        Game(Game&&) = delete;
        Game& operator=(Game&&) = delete;

        void SetDefaultDogSpeed(double speed) noexcept;
        double GetDefaultDogSpeed() const noexcept;
        const Maps& GetMaps() const noexcept;
        const Map* FindMap(const Map::Id& id) const noexcept;

        void AddMap(Map map);
        void SetRandomSpawnPoints(bool randomize) noexcept;
        bool IsRandomSpawnPoints() const noexcept;

        std::shared_ptr<Player> JoinGame(const std::string& player_name, const Map::Id& map_id);
        const std::vector<std::shared_ptr<Player>>& GetPlayers() const;
        std::shared_ptr<Player> FindPlayerByToken(const Token& token);
        void UpdateState(int delta_time);

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        double default_dog_speed_;
        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        Players players_;
        TokenGenerator token_generator_;
        bool randomize_spawn_points_;
        std::random_device random_device_;
        std::mt19937_64 generator_;
    };

} // namespace model
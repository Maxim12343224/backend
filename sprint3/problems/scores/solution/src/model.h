#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <random>
#include <atomic>
#include <cmath>
#include <mutex>
#include <optional>
#include <algorithm>
#include <chrono>
#include <iostream>

#include "tagged.h"
#include "loot_generator.h"
#include "collision_detector.h"

namespace model {

    using Dimension = double;
    using Coord = double;

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Offset {
        Dimension dx, dy;
    };

    enum class Direction {
        North,
        South,
        West,
        East
    };

    class Road {
        struct HorizontalTag {
            HorizontalTag() = default;
        };
        struct VerticalTag {
            VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{ start }, end_{ end_x, start.y } {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{ start }, end_{ start.x, end_y } {
        }

        bool IsHorizontal() const noexcept { return start_.y == end_.y; }
        bool IsVertical() const noexcept { return start_.x == end_.x; }
        Point GetStart() const noexcept { return start_; }
        Point GetEnd() const noexcept { return end_; }

        Rectangle GetBoundingBox() const noexcept {
            if (IsHorizontal()) {
                Coord x0 = std::min(start_.x, end_.x);
                Coord x1 = std::max(start_.x, end_.x);
                return { {x0, start_.y - 0.4}, {x1 - x0, 0.8} };
            }
            else {
                Coord y0 = std::min(start_.y, end_.y);
                Coord y1 = std::max(start_.y, end_.y);
                return { {start_.x - 0.4, y0}, {0.8, y1 - y0} };
            }
        }

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(Rectangle bounds) noexcept : bounds_{ bounds } {}
        const Rectangle& GetBounds() const noexcept { return bounds_; }

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{ std::move(id) }, position_{ position }, offset_{ offset } {
        }

        const Id& GetId() const noexcept { return id_; }
        Point GetPosition() const noexcept { return position_; }
        Offset GetOffset() const noexcept { return offset_; }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept
            : id_(std::move(id)), name_(std::move(name)) {
        }

        const Id& GetId() const noexcept { return id_; }
        const std::string& GetName() const noexcept { return name_; }
        const Buildings& GetBuildings() const noexcept { return buildings_; }
        const Roads& GetRoads() const noexcept { return roads_; }
        const Offices& GetOffices() const noexcept { return offices_; }
        const std::optional<double>& GetDogSpeed() const noexcept { return dog_speed_; }

        void AddRoad(const Road& road) { roads_.emplace_back(road); }
        void AddBuilding(const Building& building) { buildings_.emplace_back(building); }
        void AddOffice(Office office);

        void SetDogSpeed(double speed) noexcept { dog_speed_ = speed; }

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
        std::optional<double> dog_speed_;
    };

    class Dog {
    public:
        Dog(std::string name, Point start_pos, Direction start_dir = Direction::North)
            : name_(std::move(name)),
            position_(start_pos),
            speed_{ 0.0, 0.0 },
            direction_(start_dir) {
        }

        const std::string& GetName() const noexcept { return name_; }
        Point GetPosition() const noexcept { return position_; }
        Point GetSpeed() const noexcept { return speed_; }
        Direction GetDirection() const noexcept { return direction_; }
        void SetSpeed(Point speed) noexcept { speed_ = speed; }
        void SetDirection(Direction dir) noexcept { direction_ = dir; }
        void SetPosition(Point p) noexcept { position_ = p; }

    private:
        std::string name_;
        Point position_;
        Point speed_;
        Direction direction_;
    };

    struct LostObject {
        size_t id;
        size_t type;
        Point position;
        int value;  // Добавляем поле стоимости предмета
    };

    class GameSession;

    class Player {
    public:
        using Id = util::Tagged<uint32_t, Player>;
        using Token = util::Tagged<std::string, Player>;

        Player(std::shared_ptr<GameSession> session, Dog dog, uint32_t id, std::string token, size_t bag_capacity)
            : id_(Id{ id }), token_(Token{ std::move(token) }), dog_(std::move(dog)), session_(std::move(session)), bag_capacity_(bag_capacity) {
        }

        const Id& GetId() const noexcept { return id_; }
        const Token& GetToken() const noexcept { return token_; }
        const Dog& GetDog() const noexcept { return dog_; }
        Dog& GetDog() noexcept { return dog_; }
        const std::shared_ptr<GameSession>& GetSession() const noexcept { return session_; }
        const std::vector<LostObject>& GetBag() const noexcept { return bag_; }
        size_t GetBagCapacity() const noexcept { return bag_capacity_; }
        bool IsBagFull() const noexcept { return bag_.size() >= bag_capacity_; }
        int GetScore() const noexcept { return score_; }  // Добавляем метод для получения очков

        bool AddItemToBag(const LostObject& item) {
            if (IsBagFull()) return false;
            bag_.push_back(item);
            return true;
        }

        void ClearBag() { bag_.clear(); }
        void AddScore(int points) { score_ += points; }  // Добавляем метод для добавления очков

    private:
        Id id_;
        Token token_;
        Dog dog_;
        std::shared_ptr<GameSession> session_;
        std::vector<LostObject> bag_;
        size_t bag_capacity_;
        int score_ = 0;  // Добавляем поле для хранения очков
    };

    class GameSession : public std::enable_shared_from_this<GameSession> {
    public:
        using Id = util::Tagged<uint32_t, GameSession>;

        explicit GameSession(Map map, uint32_t id, double dog_speed, bool randomize_spawn_points,
            std::shared_ptr<loot_gen::LootGenerator> loot_generator, size_t bag_capacity)
            : id_(Id{ id }), map_(std::move(map)), dog_speed_(dog_speed),
            randomize_spawn_points_(randomize_spawn_points), loot_generator_(std::move(loot_generator)),
            bag_capacity_(bag_capacity) {
        }

        const Id& GetId() const noexcept { return id_; }
        const Map& GetMap() const noexcept { return map_; }
        const std::vector<std::shared_ptr<Player>>& GetPlayers() const noexcept { return players_; }
        const std::vector<LostObject>& GetLostObjects() const noexcept { return lost_objects_; }
        double GetDogSpeed() const noexcept { return dog_speed_; }

        size_t GetLootTypesCount() const noexcept { return loot_types_count_; }
        void SetLootTypesCount(size_t count) noexcept { loot_types_count_ = count; }
        void SetLootValues(const std::vector<int>& values) { loot_values_ = values; }  // Добавляем метод для установки стоимостей

        Point GenerateRandomPosition() const;
        std::shared_ptr<Player> AddPlayer(std::string dog_name);
        void SetPlayerAction(const Player::Token& token, const std::string& move);
        void GenerateLoot(std::chrono::milliseconds delta_time);
        void Tick(double delta_time);

    private:
        Id id_;
        Map map_;
        std::vector<std::shared_ptr<Player>> players_;
        std::vector<LostObject> lost_objects_;
        double dog_speed_;
        bool randomize_spawn_points_;
        size_t loot_types_count_ = 0;
        std::shared_ptr<loot_gen::LootGenerator> loot_generator_;
        std::atomic<size_t> next_lost_object_id_{ 0 };
        mutable std::recursive_mutex mutex_;
        size_t bag_capacity_;
        std::vector<int> loot_values_;  // Добавляем поле для хранения стоимостей предметов
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        Game() = default;
        explicit Game(double default_dog_speed, size_t default_bag_capacity = 3)
            : default_dog_speed_(default_dog_speed), default_bag_capacity_(default_bag_capacity) {}

        void AddMap(Map map);
        void SetDefaultDogSpeed(double speed) noexcept { default_dog_speed_ = speed; }
        double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }
        void SetDefaultBagCapacity(size_t capacity) noexcept { default_bag_capacity_ = capacity; }
        size_t GetDefaultBagCapacity() const noexcept { return default_bag_capacity_; }
        void SetRandomizeSpawnPoints(bool randomize) noexcept {
            randomize_spawn_points_ = randomize;
        }

        void SetLootGeneratorConfig(std::chrono::milliseconds period, double probability) {
            loot_generator_ = std::make_shared<loot_gen::LootGenerator>(period, probability);
        }

        std::shared_ptr<loot_gen::LootGenerator> GetLootGenerator() const noexcept {
            return loot_generator_;
        }

        void SetMapLootTypesCount(const Map::Id& map_id, size_t count) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            map_loot_types_count_[map_id] = count;
        }

        size_t GetMapLootTypesCount(const Map::Id& map_id) const {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (auto it = map_loot_types_count_.find(map_id); it != map_loot_types_count_.end()) {
                return it->second;
            }
            return 0;
        }

        void SetMapBagCapacity(const Map::Id& map_id, size_t capacity) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            map_bag_capacity_[map_id] = capacity;
        }

        size_t GetMapBagCapacity(const Map::Id& map_id) const {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (auto it = map_bag_capacity_.find(map_id); it != map_bag_capacity_.end()) {
                return it->second;
            }
            return default_bag_capacity_;
        }

        void SetMapLootValues(const Map::Id& map_id, const std::vector<int>& values) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            map_loot_values_[map_id] = values;
        }

        std::vector<int> GetMapLootValues(const Map::Id& map_id) const {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (auto it = map_loot_values_.find(map_id); it != map_loot_values_.end()) {
                return it->second;
            }
            return {};
        }

        const Maps& GetMaps() const noexcept { return maps_; }
        const Map* FindMap(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        std::shared_ptr<GameSession> FindSession(const Map::Id& map_id) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
                return it->second;
            }
            return nullptr;
        }

        std::shared_ptr<Player> JoinGame(const Map::Id& map_id, std::string dog_name);

        std::shared_ptr<Player> FindPlayerByToken(const Player::Token& token) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);

            if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
                return it->second;
            }
            return nullptr;
        }

        void SetPlayerAction(const Player::Token& token, const std::string& move) {
            auto player = FindPlayerByToken(token);
            if (!player) return;
            auto session = player->GetSession();
            if (session) {
                session->SetPlayerAction(token, move);
            }
        }

        void Tick(double delta_time) {
            std::vector<std::shared_ptr<GameSession>> sessions;
            {
                std::lock_guard<std::recursive_mutex> lock(mutex_);
                for (const auto& [map_id, session] : map_id_to_session_) {
                    sessions.push_back(session);
                }
            }

            for (auto& session : sessions) {
                session->Tick(delta_time);
            }
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> map_id_to_session_;
        std::unordered_map<Player::Token, std::shared_ptr<Player>, util::TaggedHasher<Player::Token>> token_to_player_;
        std::unordered_map<Map::Id, size_t, MapIdHasher> map_loot_types_count_;
        std::unordered_map<Map::Id, size_t, MapIdHasher> map_bag_capacity_;
        std::unordered_map<Map::Id, std::vector<int>, MapIdHasher> map_loot_values_;  // Добавляем поле для хранения стоимостей
        std::shared_ptr<loot_gen::LootGenerator> loot_generator_;
        double default_dog_speed_ = 1.0;
        size_t default_bag_capacity_ = 3;
        bool randomize_spawn_points_ = false;
        mutable std::recursive_mutex mutex_;
        std::atomic<uint32_t> next_session_id_{ 0 };
    };

    std::string DirectionToString(Direction dir);

}  // namespace model
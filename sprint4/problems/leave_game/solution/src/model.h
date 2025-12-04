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
#include <chrono>
#include <iostream>
#include <pqxx/pqxx>
#include <condition_variable>
#include <iomanip>
#include <sstream>

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

        void SetSpeed(Point speed) noexcept {
            speed_ = speed;
        }

        void SetDirection(Direction dir) noexcept { direction_ = dir; }

        void SetPosition(Point p) noexcept {
            position_ = p;
        }

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
        int value;
    };

    class GameSession;

    class Player {
    public:
        using Id = util::Tagged<uint32_t, Player>;
        using Token = util::Tagged<std::string, Player>;

        Player(std::shared_ptr<GameSession> session, Dog dog, uint32_t id, std::string token, size_t bag_capacity)
            : id_(Id{ id }), token_(Token{ std::move(token) }), dog_(std::move(dog)),
            session_(std::move(session)), bag_capacity_(bag_capacity),
            join_time_(0.0), last_move_time_(0.0), retirement_time_(0.0), is_retired_(false) {
        }

        const Id& GetId() const noexcept { return id_; }
        const Token& GetToken() const noexcept { return token_; }
        const Dog& GetDog() const noexcept { return dog_; }
        Dog& GetDog() noexcept { return dog_; }
        const std::shared_ptr<GameSession>& GetSession() const noexcept { return session_; }
        const std::vector<LostObject>& GetBag() const noexcept { return bag_; }
        size_t GetBagCapacity() const noexcept { return bag_capacity_; }
        bool IsBagFull() const noexcept { return bag_.size() >= bag_capacity_; }
        int GetScore() const noexcept { return score_; }

        double GetJoinTime() const noexcept { return join_time_; }
        double GetLastMoveTime() const noexcept { return last_move_time_; }
        double GetRetirementTime() const noexcept { return retirement_time_; }

        void SetJoinTime(double time) noexcept { join_time_ = time; }
        void SetLastMoveTime(double time) noexcept { last_move_time_ = time; }
        void SetRetirementTime(double time) noexcept { retirement_time_ = time; }

        bool IsRetired() const noexcept { return is_retired_; }

        void Retire(double current_time) noexcept {
            is_retired_ = true;
            retirement_time_ = current_time;
        }

        bool AddItemToBag(const LostObject& item) {
            if (IsBagFull()) return false;
            bag_.push_back(item);
            return true;
        }

        void ClearBag() { bag_.clear(); }
        void AddScore(int points) { score_ += points; }

    private:
        Id id_;
        Token token_;
        Dog dog_;
        std::shared_ptr<GameSession> session_;
        std::vector<LostObject> bag_;
        size_t bag_capacity_;
        int score_ = 0;
        double join_time_;
        double last_move_time_;
        double retirement_time_;
        bool is_retired_;
    };

    class ConnectionPool {
        using PoolType = ConnectionPool;
        using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    public:
        class ConnectionWrapper {
        public:
            ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
                : conn_{ std::move(conn) }
                , pool_{ &pool } {
            }

            ConnectionWrapper(const ConnectionWrapper&) = delete;
            ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

            ConnectionWrapper(ConnectionWrapper&&) = default;
            ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

            pqxx::connection& operator*() const& noexcept {
                return *conn_;
            }

            pqxx::connection* operator->() const& noexcept {
                return conn_.get();
            }

            ~ConnectionWrapper() {
                if (conn_) {
                    pool_->ReturnConnection(std::move(conn_));
                }
            }

        private:
            std::shared_ptr<pqxx::connection> conn_;
            PoolType* pool_;
        };

        template <typename ConnectionFactory>
        ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
            pool_.reserve(capacity);
            for (size_t i = 0; i < capacity; ++i) {
                pool_.emplace_back(connection_factory());
            }
        }

        ConnectionWrapper GetConnection() {
            std::unique_lock lock{ mutex_ };
            cond_var_.wait(lock, [this] {
                return used_connections_ < pool_.size();
                });
            return { std::move(pool_[used_connections_++]), *this };
        }

        bool IsInitialized() const {
            return !pool_.empty();
        }

    private:
        void ReturnConnection(ConnectionPtr&& conn) {
            {
                std::lock_guard lock{ mutex_ };
                pool_[--used_connections_] = std::move(conn);
            }
            cond_var_.notify_one();
        }

        std::mutex mutex_;
        std::condition_variable cond_var_;
        std::vector<ConnectionPtr> pool_;
        size_t used_connections_ = 0;
    };

    class RetiredPlayersRepository {
    public:
        RetiredPlayersRepository(const std::string& db_url) : db_url_(db_url) {
            Initialize();
        }

        void AddRetiredPlayer(const std::string& name, int score, double play_time) {
            if (use_memory_) {
                AddToMemory(name, score, play_time);
                return;
            }

            try {
                auto conn = pool_->GetConnection();
                pqxx::work txn(*conn);
                txn.exec_params(
                    "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)",
                    name, score, play_time
                );
                txn.commit();
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to add retired player to PostgreSQL: " << e.what() << std::endl;
                use_memory_ = true;
                AddToMemory(name, score, play_time);
            }
        }

        std::vector<std::tuple<std::string, int, double>> GetRecords(int start = 0, int max_items = 100) {
            if (max_items > 100) max_items = 100;
            if (max_items < 0) max_items = 100;
            if (start < 0) start = 0;

            if (use_memory_) {
                return GetFromMemory(start, max_items);
            }

            try {
                auto conn = pool_->GetConnection();
                pqxx::work txn(*conn);
                auto result = txn.exec_params(
                    "SELECT name, score, play_time FROM retired_players "
                    "ORDER BY score DESC, play_time ASC, name ASC "
                    "LIMIT $1 OFFSET $2",
                    max_items, start
                );

                std::vector<std::tuple<std::string, int, double>> records;
                for (const auto& row : result) {
                    records.emplace_back(
                        row["name"].as<std::string>(),
                        row["score"].as<int>(),
                        row["play_time"].as<double>()
                    );
                }
                return records;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to get records from PostgreSQL: " << e.what() << std::endl;
                use_memory_ = true;
                return GetFromMemory(start, max_items);
            }
        }

    private:
        void Initialize() {
            if (db_url_.empty()) {
                use_memory_ = true;
                return;
            }

            try {
                pool_ = std::make_unique<ConnectionPool>(5, [this] {
                    auto conn = std::make_shared<pqxx::connection>(db_url_);
                    return conn;
                    });

                auto conn = pool_->GetConnection();
                pqxx::work txn(*conn);
                txn.exec(
                    "CREATE TABLE IF NOT EXISTS retired_players ("
                    "id SERIAL PRIMARY KEY,"
                    "name TEXT NOT NULL,"
                    "score INTEGER NOT NULL,"
                    "play_time DOUBLE PRECISION NOT NULL,"
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                    ")"
                );
                txn.exec(
                    "CREATE INDEX IF NOT EXISTS idx_retired_players_score ON retired_players (score DESC, play_time ASC, name ASC)"
                );
                txn.commit();

            }
            catch (const std::exception& e) {
                std::cerr << "Failed to initialize PostgreSQL connection pool: " << e.what() << std::endl;
                use_memory_ = true;
            }
        }

        void AddToMemory(const std::string& name, int score, double play_time) {
            std::lock_guard lock(memory_mutex_);
            memory_records_.emplace_back(name, score, play_time);

            std::sort(memory_records_.begin(), memory_records_.end(),
                [](const auto& a, const auto& b) {
                    if (std::get<1>(a) != std::get<1>(b))
                        return std::get<1>(a) > std::get<1>(b);
                    if (std::get<2>(a) != std::get<2>(b))
                        return std::get<2>(a) < std::get<2>(b);
                    return std::get<0>(a) < std::get<0>(b);
                });
        }

        std::vector<std::tuple<std::string, int, double>> GetFromMemory(int start, int max_items) {
            std::lock_guard lock(memory_mutex_);

            if (start < 0) start = 0;
            size_t start_idx = static_cast<size_t>(start);

            if (start_idx >= memory_records_.size()) {
                return {};
            }

            size_t end_idx = start_idx + static_cast<size_t>(max_items);
            if (end_idx > memory_records_.size()) {
                end_idx = memory_records_.size();
            }

            std::vector<std::tuple<std::string, int, double>> result;
            result.reserve(end_idx - start_idx);

            for (size_t i = start_idx; i < end_idx; ++i) {
                result.push_back(memory_records_[i]);
            }

            return result;
        }

        std::string db_url_;
        std::unique_ptr<ConnectionPool> pool_;
        bool use_memory_ = false;
        std::vector<std::tuple<std::string, int, double>> memory_records_;
        std::mutex memory_mutex_;
    };

    struct RetiredPlayerRecord {
        std::string name;
        int score;
        double play_time;

        RetiredPlayerRecord(std::string n, int s, double pt)
            : name(std::move(n)), score(s), play_time(pt) {}
    };

    class GameSession : public std::enable_shared_from_this<GameSession> {
    public:
        using Id = util::Tagged<uint32_t, GameSession>;
        using RetiredPlayersCallback = std::function<void(const std::vector<std::shared_ptr<Player>>&)>;

        explicit GameSession(Map map, uint32_t id, double dog_speed, bool randomize_spawn_points,
            std::shared_ptr<loot_gen::LootGenerator> loot_generator, size_t bag_capacity,
            std::chrono::milliseconds retirement_time)
            : id_(Id{ id }), map_(std::move(map)), dog_speed_(dog_speed),
            randomize_spawn_points_(randomize_spawn_points), loot_generator_(std::move(loot_generator)),
            bag_capacity_(bag_capacity), retirement_time_(retirement_time), current_game_time_(0.0) {
        }

        const Id& GetId() const noexcept { return id_; }
        const Map& GetMap() const noexcept { return map_; }
        const std::vector<std::shared_ptr<Player>>& GetPlayers() const noexcept { return players_; }
        std::vector<std::shared_ptr<Player>> GetActivePlayers() const;
        const std::vector<LostObject>& GetLostObjects() const noexcept { return lost_objects_; }
        double GetDogSpeed() const noexcept { return dog_speed_; }
        std::chrono::milliseconds GetRetirementTime() const noexcept { return retirement_time_; }
        double GetCurrentGameTime() const noexcept { return current_game_time_; }

        size_t GetLootTypesCount() const noexcept { return loot_types_count_; }
        void SetLootTypesCount(size_t count) noexcept { loot_types_count_ = count; }
        void SetLootValues(const std::vector<int>& values) { loot_values_ = values; }
        void SetRetiredPlayersCallback(RetiredPlayersCallback callback);

        Point GenerateRandomPosition() const;
        std::shared_ptr<Player> AddPlayer(std::string dog_name);
        void SetPlayerAction(const Player::Token& token, const std::string& move);
        void GenerateLoot(std::chrono::milliseconds delta_time);
        void Tick(double delta_time);
        void UpdateGameTime(double delta_time_seconds);
        void CheckRetiredPlayers(double delta_time_seconds);

        void AddRestoredPlayer(std::shared_ptr<Player> player);
        void AddRestoredLostObject(const LostObject& obj);
        void SetNextLostObjectId(size_t id);
        size_t GetNextLostObjectId() const;

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
        std::vector<int> loot_values_;
        std::chrono::milliseconds retirement_time_;
        double current_game_time_;
        RetiredPlayersCallback retired_callback_;
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        Game() : default_dog_speed_(1.0), default_bag_capacity_(3) {
            retired_repo_ = std::make_unique<RetiredPlayersRepository>("");
        }

        explicit Game(const std::string& db_url, double default_dog_speed, size_t default_bag_capacity = 3)
            : default_dog_speed_(default_dog_speed), default_bag_capacity_(default_bag_capacity) {
            if (!db_url.empty()) {
                retired_repo_ = std::make_unique<RetiredPlayersRepository>(db_url);
            }
            else {
                retired_repo_ = std::make_unique<RetiredPlayersRepository>("");
            }
        }

        void SetDatabaseUrl(const std::string& db_url) {
            if (!db_url.empty()) {
                retired_repo_ = std::make_unique<RetiredPlayersRepository>(db_url);
            }
        }

        void AddMap(Map map);
        void SetDefaultDogSpeed(double speed) noexcept { default_dog_speed_ = speed; }
        double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }
        void SetDefaultBagCapacity(size_t capacity) noexcept { default_bag_capacity_ = capacity; }
        size_t GetDefaultBagCapacity() const noexcept { return default_bag_capacity_; }
        void SetRandomizeSpawnPoints(bool randomize) noexcept {
            randomize_spawn_points_ = randomize;
        }

        void SetRetirementTime(std::chrono::milliseconds time) noexcept {
            retirement_time_ = time;
        }
        std::chrono::milliseconds GetRetirementTime() const noexcept {
            return retirement_time_;
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

        std::shared_ptr<const GameSession> FindSession(const Map::Id& map_id) const {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
                return it->second;
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
                auto player = it->second;
                if (!player->IsRetired()) {
                    return player;
                }
            }
            return nullptr;
        }

        std::vector<std::shared_ptr<Player>> GetAllPlayers() const;

        void AddRetiredPlayer(std::shared_ptr<Player> player);

        void HandlePlayerRetirement(const std::vector<std::shared_ptr<Player>>& retired_players);

        std::vector<RetiredPlayerRecord> GetRetiredPlayers(int start = 0, int max_items = 100) {
            if (!retired_repo_) {
                return {};
            }

            auto db_records = retired_repo_->GetRecords(start, max_items);
            std::vector<RetiredPlayerRecord> records;

            for (const auto& [name, score, play_time] : db_records) {
                records.emplace_back(name, score, play_time);
            }

            return records;
        }

        void SetPlayerAction(const Player::Token& token, const std::string& move) {
            auto player = FindPlayerByToken(token);
            if (!player) return;
            auto session = player->GetSession();
            if (session) {
                session->SetPlayerAction(token, move);
            }
        }

        void Tick(double delta_time);

        void AddRestoredSession(const Map::Id& map_id, std::shared_ptr<GameSession> session);
        void RestoreTokenToPlayerMapping(const Player::Token& token, std::shared_ptr<Player> player);

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::unique_ptr<RetiredPlayersRepository> retired_repo_;
        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> map_id_to_session_;
        std::unordered_map<Player::Token, std::shared_ptr<Player>, util::TaggedHasher<Player::Token>> token_to_player_;
        std::unordered_map<Map::Id, size_t, MapIdHasher> map_loot_types_count_;
        std::unordered_map<Map::Id, size_t, MapIdHasher> map_bag_capacity_;
        std::unordered_map<Map::Id, std::vector<int>, MapIdHasher> map_loot_values_;
        std::shared_ptr<loot_gen::LootGenerator> loot_generator_;
        double default_dog_speed_ = 1.0;
        size_t default_bag_capacity_ = 3;
        bool randomize_spawn_points_ = false;
        mutable std::recursive_mutex mutex_;
        std::atomic<uint32_t> next_session_id_{ 0 };
        std::chrono::milliseconds retirement_time_{ 60000 };
    };

    std::string DirectionToString(Direction dir);

}  // namespace model
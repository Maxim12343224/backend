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

#include "tagged.h"

namespace model {

    using Dimension = double;  // Изменено int -> double
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
        North,  // U
        South,  // D
        West,   // L
        East    // R
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
                Coord x0 = std::min(start_.x, end_.x) - 0.4;
                Coord x1 = std::max(start_.x, end_.x) + 0.4;
                return { {x0, start_.y - 0.4}, {x1 - x0, 0.8} };
            }
            else {
                Coord y0 = std::min(start_.y, end_.y) - 0.4;
                Coord y1 = std::max(start_.y, end_.y) + 0.4;
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

    class GameSession;

    class Player {
    public:
        using Id = util::Tagged<uint32_t, Player>;
        using Token = util::Tagged<std::string, Player>;

        Player(std::shared_ptr<GameSession> session, Dog dog, uint32_t id, std::string token)
            : id_(Id{ id }), token_(Token{ std::move(token) }), dog_(std::move(dog)), session_(std::move(session)) {
        }

        const Id& GetId() const noexcept { return id_; }
        const Token& GetToken() const noexcept { return token_; }
        const Dog& GetDog() const noexcept { return dog_; }
        Dog& GetDog() noexcept { return dog_; }
        const std::shared_ptr<GameSession>& GetSession() const noexcept { return session_; }

    private:
        Id id_;
        Token token_;
        Dog dog_;
        std::shared_ptr<GameSession> session_;
    };

    class GameSession : public std::enable_shared_from_this<GameSession> {
    public:
        using Id = util::Tagged<uint32_t, GameSession>;

        explicit GameSession(Map map, uint32_t id, double dog_speed)
            : id_(Id{ id }), map_(std::move(map)), dog_speed_(dog_speed) {
        }

        const Id& GetId() const noexcept { return id_; }
        const Map& GetMap() const noexcept { return map_; }
        const std::vector<std::shared_ptr<Player>>& GetPlayers() const noexcept { return players_; }
        double GetDogSpeed() const noexcept { return dog_speed_; }

        Point GenerateRandomPosition() const;
        std::shared_ptr<Player> AddPlayer(std::string dog_name);
        void SetPlayerAction(const Player::Token& token, const std::string& move);
        void Tick(double delta_time);

    private:
        Id id_;
        Map map_;
        std::vector<std::shared_ptr<Player>> players_;
        double dog_speed_;
        mutable std::mutex mutex_;
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        Game() = default;
        explicit Game(double default_dog_speed) : default_dog_speed_(default_dog_speed) {}

        void AddMap(Map map);
        void SetDefaultDogSpeed(double speed) noexcept { default_dog_speed_ = speed; }
        double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }

        const Maps& GetMaps() const noexcept { return maps_; }
        const Map* FindMap(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        std::shared_ptr<GameSession> FindSession(const Map::Id& map_id) {
            if (auto it = map_id_to_session_.find(map_id); it != map_id_to_session_.end()) {
                return it->second;
            }
            return nullptr;
        }

        std::shared_ptr<GameSession> CreateSession(const Map::Id& map_id) {
            if (const auto* map = FindMap(map_id)) {
                static std::atomic<uint32_t> next_session_id_{ 0 };
                double dog_speed = map->GetDogSpeed().value_or(default_dog_speed_);
                auto session = std::make_shared<GameSession>(*map, next_session_id_++, dog_speed);
                map_id_to_session_[map_id] = session;
                return session;
            }
            return nullptr;
        }

        std::shared_ptr<Player> JoinGame(const Map::Id& map_id, std::string dog_name) {
            auto session = FindSession(map_id);
            if (!session) {
                session = CreateSession(map_id);
                if (!session) return nullptr;
            }
            auto player = session->AddPlayer(std::move(dog_name));
            token_to_player_[player->GetToken()] = player;
            return player;
        }

        std::shared_ptr<Player> FindPlayerByToken(const Player::Token& token) {
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
            for (auto& [_, session] : map_id_to_session_) {
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
        double default_dog_speed_ = 1.0;
    };

    std::string DirectionToString(Direction dir);

}  // namespace model
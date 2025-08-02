#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <random>
#include <atomic>

#include "tagged.h"

namespace model {

    using Dimension = int;
    using Coord = Dimension;

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

        void AddRoad(const Road& road) { roads_.emplace_back(road); }
        void AddBuilding(const Building& building) { buildings_.emplace_back(building); }
        void AddOffice(Office office);

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
    };

    class Dog {
    public:
        explicit Dog(std::string name) : name_(std::move(name)) {}
        const std::string& GetName() const noexcept { return name_; }

    private:
        std::string name_;
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

        explicit GameSession(Map map, uint32_t id)
            : id_(Id{ id }), map_(std::move(map)) {
        }

        const Id& GetId() const noexcept { return id_; }
        const Map& GetMap() const noexcept { return map_; }
        const std::vector<std::shared_ptr<Player>>& GetPlayers() const noexcept { return players_; }

        std::shared_ptr<Player> AddPlayer(Dog dog) {
            static std::atomic<uint32_t> next_player_id_{ 0 };

            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, 15);
            const char* hex_digits = "0123456789abcdef";
            std::string token;
            token.reserve(32);
            for (int i = 0; i < 32; ++i) {
                token += hex_digits[dis(gen)];
            }

            auto player = std::make_shared<Player>(shared_from_this(), std::move(dog),
                next_player_id_++, std::move(token));
            players_.push_back(player);
            return player;
        }

    private:
        Id id_;
        Map map_;
        std::vector<std::shared_ptr<Player>> players_;
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        void AddMap(Map map);

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
                auto session = std::make_shared<GameSession>(*map, next_session_id_++);
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

            auto player = session->AddPlayer(Dog(std::move(dog_name)));
            token_to_player_[player->GetToken()] = player;
            return player;
        }

        const std::shared_ptr<Player> FindPlayerByToken(const Player::Token& token) const {
            if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
                return it->second;
            }
            return nullptr;
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> map_id_to_session_;
        std::unordered_map<Player::Token, std::shared_ptr<Player>, util::TaggedHasher<Player::Token>> token_to_player_;
    };

}  // namespace model
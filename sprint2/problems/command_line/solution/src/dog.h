#pragma once
#include "model.h"
#include "map.h"  // ƒобавлено дл€ доступа к полному объ€влению Map
#include <array>
#include <optional>

namespace model {

    class Dog {
    public:
        using Id = util::Tagged<std::string, Dog>;

        enum class Direction {
            NORTH = 'U',
            SOUTH = 'D',
            WEST = 'L',
            EAST = 'R'
        };

        Dog(Id id, std::string name, Point spawn_point)
            : id_(std::move(id)), name_(std::move(name)),
            position_{ static_cast<double>(spawn_point.x), static_cast<double>(spawn_point.y) } {}

        const Id& GetId() const noexcept { return id_; }
        const std::string& GetName() const noexcept { return name_; }

        std::array<double, 2> GetPosition() const noexcept { return position_; }
        std::array<double, 2> GetSpeed() const noexcept { return speed_; }
        Direction GetDirection() const noexcept { return direction_; }
        const Map* GetMap() const noexcept { return map_; }

        void SetPosition(double x, double y) noexcept {
            position_[0] = x;
            position_[1] = y;
        }

        void SetSpeed(double vx, double vy) noexcept {
            speed_[0] = vx;
            speed_[1] = vy;
            if (vx == 0 && vy == 0) {
                return;
            }
            if (std::abs(vx) > std::abs(vy)) {
                direction_ = vx > 0 ? Direction::EAST : Direction::WEST;
            }
            else {
                direction_ = vy > 0 ? Direction::SOUTH : Direction::NORTH;
            }
        }

        void SetDirection(Direction dir) noexcept {
            direction_ = dir;
        }

        void SetMap(const Map* map) noexcept {
            map_ = map;
        }

        // ќбъ€вление без реализации (реализаци€ будет позже)
        void UpdatePosition(double delta_time);

    private:
        Id id_;
        std::string name_;
        std::array<double, 2> position_;
        std::array<double, 2> speed_{ 0.0, 0.0 };
        Direction direction_{ Direction::NORTH };
        const Map* map_{ nullptr };
    };

    // –еализаци€ после объ€влени€ класса Map
    inline void Dog::UpdatePosition(double delta_time) {
        if (speed_[0] == 0 && speed_[1] == 0) {
            return;
        }

        double new_x = position_[0] + speed_[0] * delta_time / 1000.0;
        double new_y = position_[1] + speed_[1] * delta_time / 1000.0;

        if (map_) {
            auto [clamped_x, clamped_y] = map_->ClampPosition(position_[0], position_[1], new_x, new_y);
            if (clamped_x != new_x || clamped_y != new_y) {
                SetSpeed(0, 0);
            }
            position_[0] = clamped_x;
            position_[1] = clamped_y;
        }
        else {
            position_[0] = new_x;
            position_[1] = new_y;
        }
    }

} // namespace model
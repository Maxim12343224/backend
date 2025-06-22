#include "model.h"
#include "map.h"
#include <stdexcept>
#include <cmath>

namespace model {
    // Реализация методов классов Road, Building, Office и Map
    // Road implementation
    Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_(start), end_{ end_x, start.y } {}

    Road::Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_(start), end_{ start.x, end_y } {}

    bool Road::IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool Road::IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point Road::GetStart() const noexcept {
        return start_;
    }

    Point Road::GetEnd() const noexcept {
        return end_;
    }

    // Building implementation
    Building::Building(Rectangle bounds) noexcept
        : bounds_(bounds) {}

    const Rectangle& Building::GetBounds() const noexcept {
        return bounds_;
    }

    // Office implementation
    Office::Office(Id id, Point position, Offset offset) noexcept
        : id_(std::move(id)), position_(position), offset_(offset) {}

    const Office::Id& Office::GetId() const noexcept {
        return id_;
    }

    Point Office::GetPosition() const noexcept {
        return position_;
    }

    Offset Office::GetOffset() const noexcept {
        return offset_;
    }

    // Map implementation
    Map::Map(Id id, std::string name, double dog_speed) noexcept
        : id_(std::move(id)), name_(std::move(name)), dog_speed_(dog_speed) {}

    const Map::Id& Map::GetId() const noexcept {
        return id_;
    }

    const std::string& Map::GetName() const noexcept {
        return name_;
    }

    double Map::GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    void Map::SetDogSpeed(double speed) noexcept {
        dog_speed_ = speed;
    }

    const Map::Buildings& Map::GetBuildings() const noexcept {
        return buildings_;
    }

    const Map::Roads& Map::GetRoads() const noexcept {
        return roads_;
    }

    const Map::Offices& Map::GetOffices() const noexcept {
        return offices_;
    }

    void Map::AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void Map::AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void Map::AddOffice(Office office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        }
        catch (...) {
            offices_.pop_back();
            throw;
        }
    }

    Point Map::GetSpawnPoint() const {
        if (roads_.empty()) {
            return { 0, 0 };
        }
        return { roads_[0].GetStart().x, roads_[0].GetStart().y };
    }

    std::pair<double, double> Map::ClampPosition(double old_x, double old_y, double new_x, double new_y) const {
        if (roads_.empty()) {
            return { new_x, new_y };
        }

        for (const auto& building : buildings_) {
            const auto& bounds = building.GetBounds();
            if (new_x >= bounds.position.x &&
                new_x < bounds.position.x + bounds.size.width &&
                new_y >= bounds.position.y &&
                new_y < bounds.position.y + bounds.size.height) {
                return { old_x, old_y };
            }
        }

        for (const auto& road : roads_) {
            if (road.IsHorizontal()) {
                const int min_x = std::min(road.GetStart().x, road.GetEnd().x);
                const int max_x = std::max(road.GetStart().x, road.GetEnd().x);
                if (std::abs(new_y - road.GetStart().y) < 0.5 &&
                    new_x >= min_x - 0.5 && new_x <= max_x + 0.5) {
                    return { new_x, static_cast<double>(road.GetStart().y) };
                }
            }
            else {
                const int min_y = std::min(road.GetStart().y, road.GetEnd().y);
                const int max_y = std::max(road.GetStart().y, road.GetEnd().y);
                if (std::abs(new_x - road.GetStart().x) < 0.5 &&
                    new_y >= min_y - 0.5 && new_y <= max_y + 0.5) {
                    return { static_cast<double>(road.GetStart().x), new_y };
                }
            }
        }

        return { old_x, old_y };
    }
} // namespace model
#pragma once
#include "model.h"
#include <vector>
#include <unordered_map>

namespace model {

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name, double dog_speed = 1.0) noexcept;

        const Id& GetId() const noexcept;
        const std::string& GetName() const noexcept;
        double GetDogSpeed() const noexcept;
        const Buildings& GetBuildings() const noexcept;
        const Roads& GetRoads() const noexcept;
        const Offices& GetOffices() const noexcept;

        void SetDogSpeed(double speed) noexcept;

        void AddRoad(const Road& road);
        void AddBuilding(const Building& building);
        void AddOffice(Office office);

        Point GetSpawnPoint() const;
        std::pair<double, double> ClampPosition(double old_x, double old_y, double new_x, double new_y) const;

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        double dog_speed_;
        Roads roads_;
        Buildings buildings_;
        Offices offices_;
        OfficeIdToIndex warehouse_id_to_index_;
    };

} // namespace model
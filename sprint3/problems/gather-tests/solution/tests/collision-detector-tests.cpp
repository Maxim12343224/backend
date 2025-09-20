#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "../src/collision_detector.h"

namespace collision_detector {
namespace {

class TestProvider : public ItemGathererProvider {
public:
    TestProvider(std::vector<Item> items, std::vector<Gatherer> gatherers)
        : items_(std::move(items)), gatherers_(std::move(gatherers)) {}

    size_t ItemsCount() const override { return items_.size(); }
    Item GetItem(size_t idx) const override { return items_.at(idx); }
    size_t GatherersCount() const override { return gatherers_.size(); }
    Gatherer GetGatherer(size_t idx) const override { return gatherers_.at(idx); }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

class GatheringEventMatcher : public Catch::Matchers::MatcherBase<GatheringEvent> {
public:
    GatheringEventMatcher(size_t item_id, size_t gatherer_id, double sq_distance, double time)
        : expected_{item_id, gatherer_id, sq_distance, time} {}

    bool match(const GatheringEvent& actual) const override {
        return actual.item_id == expected_.item_id &&
               actual.gatherer_id == expected_.gatherer_id &&
               actual.sq_distance == Catch::Approx(expected_.sq_distance).margin(1e-10) &&
               actual.time == Catch::Approx(expected_.time).margin(1e-10);
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "equals (" << expected_.item_id << ", " << expected_.gatherer_id << ", "
           << expected_.sq_distance << ", " << expected_.time << ")";
        return ss.str();
    }

private:
    GatheringEvent expected_;
};

GatheringEventMatcher EqualsEvent(size_t item_id, size_t gatherer_id, 
                                 double sq_distance, double time) {
    return GatheringEventMatcher(item_id, gatherer_id, sq_distance, time);
}

}  

namespace Catch {
template<>
struct StringMaker<GatheringEvent> {
    static std::string convert(GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << "," 
            << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
}  

TEST_CASE("No events when no gatherers or items", "[collision_detector]") {
    TestProvider provider({}, {});
    auto events = FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("No events when gatherer doesn't move", "[collision_detector]") {
    Gatherer stationary_gatherer{{0, 0}, {0, 0}, 0.5};
    Item item{{5, 0}, 0.5};

    TestProvider provider({item}, {stationary_gatherer});
    auto events = FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Single gatherer collects single item", "[collision_detector]") {
    Gatherer gatherer{{0, 0}, {10, 0}, 0.5};
    Item item{{5, 0}, 0.5};

    TestProvider provider({item}, {gatherer});
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0], EqualsEvent(0, 0, 0.0, 0.5));
}

TEST_CASE("Gatherer misses item by distance", "[collision_detector]") {
    Gatherer gatherer{{0, 0}, {10, 0}, 0.5};
    Item item{{5, 1.1}, 0.5};

    TestProvider provider({item}, {gatherer});
    auto events = FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Gatherer misses item by projection", "[collision_detector]") {
    Gatherer gatherer{{0, 0}, {10, 0}, 0.5};
    Item item{{15, 0}, 0.5};

    TestProvider provider({item}, {gatherer});
    auto events = FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Multiple items collected in correct order", "[collision_detector]") {
    Gatherer gatherer{{0, 0}, {10, 0}, 0.5};
    Item item1{{2, 0}, 0.5};
    Item item2{{8, 0}, 0.5};

    TestProvider provider({item1, item2}, {gatherer});
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 2);
    CHECK(events[0].time < events[1].time);
    CHECK_THAT(events[0], EqualsEvent(0, 0, 0.0, 0.2));
    CHECK_THAT(events[1], EqualsEvent(1, 0, 0.0, 0.8));
}

TEST_CASE("Multiple gatherers collect same item", "[collision_detector]") {
    Gatherer gatherer1{{0, 0}, {10, 0}, 0.5};
    Gatherer gatherer2{{0, 5}, {10, 5}, 0.5};
    Item item{{5, 0}, 0.5};

    TestProvider provider({item}, {gatherer1, gatherer2});
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0], EqualsEvent(0, 0, 0.0, 0.5));
}

TEST_CASE("Diagonal movement collection", "[collision_detector]") {
    Gatherer gatherer{{0, 0}, {10, 10}, 0.5};
    Item item{{5, 5}, 0.5};

    TestProvider provider({item}, {gatherer});
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0], EqualsEvent(0, 0, 0.0, 0.5));
}

TEST_CASE("Exact distance threshold", "[collision_detector]") {
    Gatherer gatherer{{0, 0}, {10, 0}, 0.5};
    Item item1{{5, 1.0}, 0.5};
    Item item2{{5, 1.0 + 1e-11}, 0.5};

    TestProvider provider({item1, item2}, {gatherer});
    auto events = FindGatherEvents(provider);

    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0], EqualsEvent(0, 0, 1.0, 0.5));
}

}
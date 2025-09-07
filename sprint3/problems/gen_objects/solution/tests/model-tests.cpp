#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include "../src/model.h"
#include "../src/loot_generator.h"

using namespace std::chrono_literals;

TEST_CASE("GameSession loot generation") {
    model::Map map{model::Map::Id{"test"}, "Test Map"};
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 10});
    
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(1s, 1.0);
    model::GameSession session(std::move(map), 0, 1.0, false, loot_gen);
    session.SetLootTypesCount(2);
    
    SECTION("Generate loot on empty map") {
        session.GenerateLoot(1s);
        REQUIRE(session.GetLostObjects().size() == 0);
        
        auto player = session.AddPlayer("test");
        session.GenerateLoot(1s);
        REQUIRE(session.GetLostObjects().size() == 1);
    }
    
    SECTION("Loot properties") {
        auto player = session.AddPlayer("test");
        session.GenerateLoot(1s);
        
        const auto& loot = session.GetLostObjects();
        REQUIRE(loot.size() == 1);
        REQUIRE(loot[0].type < 2);
        REQUIRE(loot[0].position.x >= 0);
        REQUIRE(loot[0].position.x <= 10);
        REQUIRE(loot[0].position.y == 0);
    }
}
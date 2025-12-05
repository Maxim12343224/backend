#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include "../src/loot_generator.h"

using namespace std::chrono_literals;
using namespace loot_gen;

TEST_CASE("LootGenerator basic functionality") {
    LootGenerator gen(1s, 1.0);
    
    SECTION("No loot generated when loot count >= looter count") {
        REQUIRE(gen.Generate(1s, 5, 5) == 0);
        REQUIRE(gen.Generate(1s, 10, 5) == 0);
    }
    
    SECTION("Generates missing loot") {
        REQUIRE(gen.Generate(1s, 0, 5) == 5);
        REQUIRE(gen.Generate(1s, 3, 5) == 2);
    }
}

TEST_CASE("LootGenerator with probability") {
    LootGenerator gen(1s, 0.5, []() { return 1.0; });
    
    SECTION("Half probability over base interval") {
        REQUIRE(gen.Generate(1s, 0, 4) == 2);
    }
    
    SECTION("Double time interval") {
        REQUIRE(gen.Generate(2s, 0, 4) == 3);
    }
}

TEST_CASE("LootGenerator with custom random") {
    LootGenerator gen(1s, 0.5, []() { return 0.0; });
    
    SECTION("Zero random generator") {
        REQUIRE(gen.Generate(1s, 0, 4) == 0);
    }
    
    LootGenerator gen_max(1s, 0.5, []() { return 1.0; });
    
    SECTION("Max random generator") {
        REQUIRE(gen_max.Generate(1s, 0, 4) == 2);
    }
}

TEST_CASE("LootGenerator time accumulation") {
    LootGenerator gen(2s, 1.0);
    
    SECTION("Multiple short intervals") {
        REQUIRE(gen.Generate(1s, 0, 4) == 0);
        REQUIRE(gen.Generate(1s, 0, 4) == 4);
        REQUIRE(gen.Generate(1s, 0, 4) == 0);
    }
}
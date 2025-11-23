#include "model.h"
#include "loot_generator.h"
#include <catch2/catch.hpp>
#include <chrono>
#include <thread>

using namespace std::literals;

TEST_CASE("Game session creation") {
    model::Map map = model::Map{model::Map::Id{"map1"}, "Map 1"};
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds(1000), 0.5);
    
    // Обновляем вызов конструктора с 7 параметрами
    model::GameSession session(std::move(map), 0, 1.0, false, loot_gen, 3, std::chrono::milliseconds(60000));
    
    REQUIRE(session.GetId() == model::GameSession::Id{0});
    REQUIRE(session.GetDogSpeed() == 1.0);
}

TEST_CASE("Player joining game") {
    model::Game game;
    auto map = model::Map{model::Map::Id{"map1"}, "Map 1"};
    game.AddMap(std::move(map));
    
    auto player = game.JoinGame(model::Map::Id{"map1"}, "Player1");
    
    REQUIRE(player != nullptr);
    REQUIRE(player->GetDog().GetName() == "Player1");
    REQUIRE(player->GetToken().length() == 32);
}

TEST_CASE("Player movement") {
    model::Map map = model::Map{model::Map::Id{"map1"}, "Map 1"};
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds(1000), 0.5);
    
    // Обновляем вызов конструктора с 7 параметрами
    model::GameSession session(std::move(map), 0, 1.0, false, loot_gen, 3, std::chrono::milliseconds(60000));
    
    auto player = session.AddPlayer("TestDog");
    REQUIRE(player != nullptr);
    
    session.SetPlayerAction(player->GetToken(), "R");
    
    // Проверяем, что скорость установлена правильно
    auto& dog = player->GetDog();
    REQUIRE(dog.GetSpeed().x == 1.0);
    REQUIRE(dog.GetSpeed().y == 0.0);
}

TEST_CASE("Game tick updates positions") {
    model::Map map = model::Map{model::Map::Id{"map1"}, "Map 1"};
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds(1000), 0.5);
    
    // Обновляем вызов конструктора с 7 параметрами
    model::GameSession session(std::move(map), 0, 1.0, false, loot_gen, 3, std::chrono::milliseconds(60000));
    
    auto player = session.AddPlayer("TestDog");
    REQUIRE(player != nullptr);
    
    auto start_pos = player->GetDog().GetPosition();
    
    session.SetPlayerAction(player->GetToken(), "R");
    session.Tick(1.0); // 1 секунда
    
    auto end_pos = player->GetDog().GetPosition();
    
    // Проверяем, что позиция изменилась
    REQUIRE(end_pos.x == start_pos.x + 1.0);
    REQUIRE(end_pos.y == start_pos.y);
}

TEST_CASE("Player score calculation") {
    model::Map map = model::Map{model::Map::Id{"map1"}, "Map 1"};
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds(1000), 0.5);
    
    model::GameSession session(std::move(map), 0, 1.0, false, loot_gen, 3, std::chrono::milliseconds(60000));
    
    auto player = session.AddPlayer("ScoringDog");
    REQUIRE(player != nullptr);
    
    // Добавляем очки
    player->AddScore(10);
    player->AddScore(5);
    
    REQUIRE(player->GetScore() == 15);
}

TEST_CASE("Player bag capacity") {
    model::Map map = model::Map{model::Map::Id{"map1"}, "Map 1"};
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds(1000), 0.5);
    
    model::GameSession session(std::move(map), 0, 1.0, false, loot_gen, 2, std::chrono::milliseconds(60000)); // вместимость 2
    
    auto player = session.AddPlayer("BagDog");
    REQUIRE(player != nullptr);
    
    // Добавляем предметы
    model::LostObject item1{1, 0, {0.0, 0.0}, 1};
    model::LostObject item2{2, 1, {0.0, 0.0}, 2};
    model::LostObject item3{3, 2, {0.0, 0.0}, 3};
    
    REQUIRE(player->AddItemToBag(item1));
    REQUIRE(player->AddItemToBag(item2));
    REQUIRE_FALSE(player->AddItemToBag(item3)); // Должно не поместиться
    
    REQUIRE(player->GetBag().size() == 2);
    REQUIRE(player->IsBagFull());
}

// Упрощенные тесты для ухода на покой (без sleep)
TEST_CASE("Player retirement state") {
    model::Map map = model::Map{model::Map::Id{"map1"}, "Map 1"};
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds(1000), 0.5);
    
    model::GameSession session(std::move(map), 0, 1.0, false, loot_gen, 3, std::chrono::milliseconds(60000));
    
    auto player = session.AddPlayer("TestDog");
    REQUIRE(player != nullptr);
    
    // Игрок не должен быть ушедшим изначально
    REQUIRE_FALSE(player->IsRetired());
    
    // Помечаем игрока как ушедшего
    player->Retire();
    
    REQUIRE(player->IsRetired());
}

TEST_CASE("Game records storage") {
    model::Game game;
    
    // Просто проверяем, что можем получить записи (даже если их нет)
    auto records = game.GetRetiredPlayers();
    REQUIRE(records.size() == 0); // Изначально записей нет
}

TEST_CASE("Retirement time configuration") {
    model::Game game;
    
    // Проверяем установку времени ухода
    auto retirement_time = std::chrono::milliseconds(30000);
    game.SetRetirementTime(retirement_time);
    
    REQUIRE(game.GetRetirementTime() == retirement_time);
}
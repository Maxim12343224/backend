cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Основной сервер
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/model.cpp
    src/model.h
    src/tagged.h
    src/boost_json.cpp
    src/sdk.h
    src/json_loader.cpp
    src/json_loader.h
    src/request_handler.cpp
    src/request_handler.h
    src/logger.h
    src/loot_generator.cpp
    src/loot_generator.h
    src/collision_detector.cpp
    src/collision_detector.h
    src/serializer.cpp
    src/serializer.h
    src/state_manager.cpp
    src/state_manager.h
)

target_link_libraries(game_server PRIVATE        
    CONAN_PKG::boost      
    Threads::Threads
    CONAN_PKG::libpq      # Добавляем PostgreSQL
    CONAN_PKG::libpqxx    # Добавляем pqxx
)

set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Тесты УДАЛЕНЫ - они не нужны для решения задачи
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
)

# target_include_directories УБРАНЫ - Conan автоматически добавляет include пути
target_link_libraries(game_server PRIVATE        
    CONAN_PKG::boost      
    Threads::Threads
)
set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Тесты
add_executable(game_server_tests
    src/model.h
    src/model.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/tagged.h
    tests/model-tests.cpp
    tests/loot_generator_tests.cpp
)
target_include_directories(game_server_tests PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads)
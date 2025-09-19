cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Создаем статическую библиотеку с общим кодом
add_library(game_lib STATIC
    src/model.cpp
    src/loot_generator.cpp
    src/tagged.h
)

# Настраиваем зависимости библиотеки
target_include_directories(game_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_lib PUBLIC
    CONAN_PKG::boost
    Threads::Threads
)

# Основной сервер
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/boost_json.cpp
    src/sdk.h
    src/json_loader.cpp
    src/json_loader.h
    src/request_handler.cpp
    src/request_handler.h
    src/logger.h
)

target_link_libraries(game_server PRIVATE
    game_lib
)

set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Тесты
add_executable(game_server_tests
    tests/model-tests.cpp
    tests/loot_generator_tests.cpp
)

target_include_directories(game_server_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_server_tests PRIVATE
    game_lib
    CONAN_PKG::catch2
)

set_target_properties(game_server_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
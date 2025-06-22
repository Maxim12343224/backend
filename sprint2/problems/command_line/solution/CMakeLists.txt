cmake_minimum_required(VERSION 3.11)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SOURCE_FILES
    src/main.cpp
    src/game.cpp
    src/game.h
    src/dog.h
    src/player.cpp
    src/player.h
    src/token_generator.cpp
    src/token_generator.h
    src/json_loader.cpp
    src/json_loader.h
    src/request_handler.cpp
    src/request_handler.h
    src/http_server.cpp
    src/http_server.h
    src/ticker.h
    src/tagged.h
    src/sdk.h
    src/logger.h
    src/model.h
    src/map.h
    src/model.cpp
    src/boost_json.cpp
)

add_executable(game_server ${SOURCE_FILES})

target_link_libraries(game_server PRIVATE
    CONAN_PKG::boost
    Threads::Threads
)

target_include_directories(game_server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

install(TARGETS game_server RUNTIME DESTINATION bin)
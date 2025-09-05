cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(SOURCE_FILES
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
        src/logger.h
)

add_executable(game_server ${SOURCE_FILES})

add_executable(game_server_tests
    src/model.h
    src/model.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/tagged.h
    tests/model-tests.cpp
    tests/loot_generator_tests.cpp
)
target_include_directories(game_server_tests PRIVATE  CONAN_PKG::boost)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads) 
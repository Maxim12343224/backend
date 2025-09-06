cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Основное приложение
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/model.cpp
    src/boost_json.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/loot_generator.cpp
)

# КРИТИЧЕСКИ ВАЖНО: добавить эти include директории
target_include_directories(game_server PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CONAN_INCLUDE_DIRS_BOOST}  # ← специально для Boost
    ${CONAN_INCLUDE_DIRS}        # ← все остальные include
)

target_link_libraries(game_server PRIVATE 
    ${CONAN_LIBS}
    CONAN_PKG::boost  # ← явно линкуем Boost
)

set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Тесты (пропускаем если файлов нет)
if(EXISTS ${CMAKE_SOURCE_DIR}/test/loot_generator_tests.cpp)
    add_executable(game_server_tests
        test/loot_generator_tests.cpp
        test/model_tests.cpp
    )

    target_include_directories(game_server_tests PRIVATE 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/test
        ${CONAN_INCLUDE_DIRS_BOOST}
        ${CONAN_INCLUDE_DIRS}
    )

    target_link_libraries(game_server_tests PRIVATE 
        ${CONAN_LIBS}
        CONAN_PKG::boost
        Catch2::Catch2WithMain
    )

    enable_testing()
    add_test(NAME game_server_tests COMMAND game_server_tests)
else()
    message(STATUS "Test files not found, skipping tests build")
endif()
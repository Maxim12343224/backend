cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Добавьте include directories для всего проекта
include_directories(${CONAN_INCLUDE_DIRS})
link_directories(${CONAN_LIB_DIRS})

# Основное приложение
add_executable(urldecode
    boost_json.cpp
    http_server.cpp
    http_server.h
    json_loader.cpp
    json_loader.h
    logger.h
    loot_generator.cpp
    loot_generator.h
    main.cpp
    model.cpp
    model.h
    request_handler.cpp
    request_handler.h
    sdk.h
    tagged.h
)

target_link_libraries(urldecode PRIVATE ${CONAN_LIBS})

# Тесты
add_executable(game_server_tests
    tests/loot_generator_tests.cpp
    tests/model_tests.cpp
    src/loot_generator.cpp
    src/loot_generator.h
    src/model.cpp
    src/model.h
    src/tagged.h
)

target_include_directories(game_server_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
    ${CONAN_INCLUDE_DIRS}
)

target_link_libraries(game_server_tests PRIVATE 
    ${CONAN_LIBS}
    Catch2::Catch2WithMain
)

# Добавляем цель для запуска тестов
enable_testing()
add_test(NAME game_server_tests COMMAND game_server_tests)
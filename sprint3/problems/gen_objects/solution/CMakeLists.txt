cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Сначала компилируем основную библиотеку
add_library(game_lib STATIC
    src/boost_json.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/loot_generator.cpp
    src/model.cpp
    src/request_handler.cpp
)

target_include_directories(game_lib PUBLIC 
    ${CMAKE_SOURCE_DIR}/src
    ${CONAN_INCLUDE_DIRS}
)

# Основное приложение
add_executable(game_server
    src/main.cpp
)

target_include_directories(game_server PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CONAN_INCLUDE_DIRS}
)

target_link_libraries(game_server PRIVATE 
    game_lib
    ${CONAN_LIBS}
)

# Устанавливаем выходной путь для основного приложения
set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Тесты - используем уже скомпилированную библиотеку
if(EXISTS ${CMAKE_SOURCE_DIR}/test/loot_generator_tests.cpp AND EXISTS ${CMAKE_SOURCE_DIR}/test/model_tests.cpp)
    add_executable(game_server_tests
        test/loot_generator_tests.cpp
        test/model_tests.cpp
    )

    target_include_directories(game_server_tests PRIVATE 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/test
        ${CONAN_INCLUDE_DIRS}
    )

    target_link_libraries(game_server_tests PRIVATE 
        game_lib
        ${CONAN_LIBS}
        Catch2::Catch2WithMain
    )

    # Добавляем цель для запуска тестов
    enable_testing()
    add_test(NAME game_server_tests COMMAND game_server_tests)
else()
    message(WARNING "Test files not found in test/ directory, skipping tests build")
endif()
cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Основное приложение - ТОЛЬКО .cpp файлы!
add_executable(urldecode
    boost_json.cpp
    http_server.cpp
    json_loader.cpp
    loot_generator.cpp
    main.cpp
    model.cpp
    request_handler.cpp
)

target_include_directories(urldecode PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CONAN_INCLUDE_DIRS}
)

target_link_libraries(urldecode PRIVATE ${CONAN_LIBS})

# Тесты - проверяем существование файлов
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/loot_generator_tests.cpp)
    add_executable(game_server_tests
        tests/loot_generator_tests.cpp
        tests/model_tests.cpp
        loot_generator.cpp
        model.cpp
    )

    target_include_directories(game_server_tests PRIVATE 
        ${CMAKE_SOURCE_DIR}
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
else()
    message(WARNING "Test files not found, skipping tests build")
endif()
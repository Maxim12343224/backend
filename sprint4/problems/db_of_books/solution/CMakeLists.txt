cmake_minimum_required(VERSION 3.11)

project(book_manager CXX)
set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Основная программа
add_executable(book_manager
    src/main.cpp
    src/book_manager.cpp
    include/book_manager.h
)

target_link_libraries(book_manager PRIVATE        
    CONAN_PKG::libpqxx
    CONAN_PKG::nlohmann_json
)

target_include_directories(book_manager PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(book_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
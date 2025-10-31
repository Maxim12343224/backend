cmake_minimum_required(VERSION 3.11)

project(book_manager CXX)
set(CMAKE_CXX_STANDARD 17)

# Скачиваем nlohmann/json если не установлен через Conan
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(book_manager
    src/main.cpp
    src/book_manager.cpp
)

target_link_libraries(book_manager 
    CONAN_PKG::libpqxx
    Threads::Threads
    nlohmann_json
)

target_include_directories(book_manager PRIVATE include)
cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Добавьте include directories для всего проекта
include_directories(${CONAN_INCLUDE_DIRS})
link_directories(${CONAN_LIB_DIRS})

add_executable(urldecode
    src/main.cpp
    src/urldecode.h
    src/urldecode.cpp
)

# Имя должно совпадать с target_link_libraries
add_executable(tests
    tests/tests.cpp
    src/urldecode.h
    src/urldecode.cpp
)

# Теперь линкуем правильный target
target_link_libraries(tests PRIVATE ${CONAN_LIBS})
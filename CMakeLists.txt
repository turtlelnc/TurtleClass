cmake_minimum_required(VERSION 3.20)
project(TurtleClass VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(TurtleClassCore
    src/core/domain.cpp
)
target_include_directories(TurtleClassCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(TurtleClassCore PUBLIC cxx_std_23)

add_executable(TurtleClassTests tests/core_tests.cpp)
target_link_libraries(TurtleClassTests PRIVATE TurtleClassCore)

enable_testing()
add_test(NAME TurtleClassCoreTests COMMAND TurtleClassTests)

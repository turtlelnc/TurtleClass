cmake_minimum_required(VERSION 3.20)
project(TurtleClass VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(TurtleClassCore
    src/core/domain.cpp
)
target_include_directories(TurtleClassCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(TurtleClassCore PUBLIC cxx_std_23)

add_library(TurtleClassSecurity
    src/security/security.cpp
)
target_include_directories(TurtleClassSecurity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TurtleClassSecurity PUBLIC TurtleClassCore)
target_compile_features(TurtleClassSecurity PUBLIC cxx_std_23)

add_library(TurtleClassServer
    src/server/server.cpp
    src/server/accounts.cpp
    src/server/snapshot.cpp
    src/server/http_server.cpp
)
target_include_directories(TurtleClassServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TurtleClassServer PUBLIC TurtleClassCore)
target_compile_features(TurtleClassServer PUBLIC cxx_std_23)

# Find SQLite3
find_package(SQLite3 REQUIRED)

add_library(TurtleClassStorage
    src/storage/storage.cpp
)
target_include_directories(TurtleClassStorage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TurtleClassStorage PUBLIC SQLite::SQLite3)
target_compile_features(TurtleClassStorage PUBLIC cxx_std_23)

add_executable(TurtleClassTests tests/core_tests.cpp tests/server_tests.cpp)
target_link_libraries(TurtleClassTests PRIVATE TurtleClassCore TurtleClassServer)

add_executable(AccountTests tests/account_tests.cpp)
target_link_libraries(AccountTests PRIVATE TurtleClassServer)

enable_testing()
add_test(NAME TurtleClassTests COMMAND TurtleClassTests)

cmake_minimum_required(VERSION 3.20)
project(TurtleClass VERSION 0.3.0 LANGUAGES CXX)

# Windows Client Selection: win32 (legacy) or winui3 (modern)
# Default: win32 for maximum compatibility
if(NOT DEFINED WINDOWS_CLIENT)
    set(WINDOWS_CLIENT "win32" CACHE STRING "Windows client type: win32 or winui3")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Building Windows client: ${WINDOWS_CLIENT}")

# Find required packages
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)
pkg_check_modules(ARGON2 REQUIRED libargon2)

add_library(TurtleClassCore
    src/core/domain.cpp
)
target_include_directories(TurtleClassCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(TurtleClassCore PUBLIC cxx_std_23)

add_library(TurtleClassSecurity
    src/security/security.cpp
)
target_include_directories(TurtleClassSecurity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TurtleClassSecurity PUBLIC TurtleClassCore ${SODIUM_LIBRARIES} ${ARGON2_LIBRARIES})
target_include_directories(TurtleClassSecurity PRIVATE ${SODIUM_INCLUDE_DIRS} ${ARGON2_INCLUDE_DIRS})
target_compile_features(TurtleClassSecurity PUBLIC cxx_std_23)

add_library(TurtleClassServer
    src/server/server.cpp
    src/server/accounts.cpp
    src/server/snapshot.cpp
    src/server/http_server.cpp
    src/server/database.cpp
    src/server/maintenance.cpp
    src/server/exception_report.cpp
)
target_include_directories(TurtleClassServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TurtleClassServer PUBLIC TurtleClassCore TurtleClassSecurity SQLite::SQLite3 ${SODIUM_LIBRARIES} ${ARGON2_LIBRARIES})
target_include_directories(TurtleClassServer PRIVATE ${SODIUM_INCLUDE_DIRS} ${ARGON2_INCLUDE_DIRS})
target_compile_features(TurtleClassServer PUBLIC cxx_std_23)

add_library(TurtleClassStorage
    src/storage/storage.cpp
)
target_include_directories(TurtleClassStorage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TurtleClassStorage PUBLIC SQLite::SQLite3)
target_compile_features(TurtleClassStorage PUBLIC cxx_std_23)

# Windows Desktop Client - Conditional Build
if(WIN32)
    if(WINDOWS_CLIENT STREQUAL "winui3")
        message(STATUS "Building WinUI 3 Modern Client")
        
        # Find Windows App SDK for WinUI 3
        find_package(Microsoft.WindowsAppSDK REQUIRED)
        
        add_executable(TurtleClassClient WIN32
            src/windows/winui3_modern/main.cpp
            src/windows/winui3_modern/MainWindow.cpp
            src/windows/winui3_modern/HttpClient.cpp
            src/windows/winui3_modern/AuthenticationService.cpp
            src/windows/winui3_modern/SyncService.cpp
            src/windows/winui3_modern/StudentListViewModel.cpp
        )
        target_link_libraries(TurtleClassClient PRIVATE 
            TurtleClassCore 
            TurtleClassStorage
            winhttp
            Microsoft.WindowsAppSDK::CppWinRT
        )
        target_compile_features(TurtleClassClient PUBLIC cxx_std_23)
        
        if(MSVC)
            target_compile_options(TurtleClassClient PRIVATE /W4 /WX- /await)
        endif()
        
    elseif(WINDOWS_CLIENT STREQUAL "win32")
        message(STATUS "Building Win32 Legacy Client")
        
        add_executable(TurtleClassClient WIN32
            src/windows/win32_legacy/TurtleClass.WindowsDesktop/main.cpp
            src/windows/win32_legacy/TurtleClass.WindowsDesktop/MainWindow.cpp
            src/windows/win32_legacy/TurtleClass.WindowsDesktop/HttpClient.cpp
            src/windows/win32_legacy/TurtleClass.WindowsDesktop/AuthenticationService.cpp
            src/windows/win32_legacy/TurtleClass.WindowsDesktop/SyncService.cpp
            src/windows/win32_legacy/TurtleClass.WindowsDesktop/StudentListViewModel.cpp
        )
        target_link_libraries(TurtleClassClient PRIVATE 
            TurtleClassCore 
            TurtleClassStorage
            winhttp
        )
        target_compile_features(TurtleClassClient PUBLIC cxx_std_23)
        
        if(MSVC)
            target_compile_options(TurtleClassClient PRIVATE /W4 /WX-)
        endif()
        
    else()
        message(FATAL_ERROR "Unknown WINDOWS_CLIENT type: ${WINDOWS_CLIENT}. Use 'win32' or 'winui3'.")
    endif()
else()
    message(STATUS "Not on Windows, skipping Windows client build")
endif()

add_executable(TurtleClassTests tests/core_tests.cpp tests/server_tests.cpp)
target_link_libraries(TurtleClassTests PRIVATE TurtleClassCore TurtleClassServer TurtleClassStorage)

add_executable(AccountTests tests/account_tests.cpp)
target_link_libraries(AccountTests PRIVATE TurtleClassServer)

# SyncSimulator disabled - missing dependencies (core/event.hpp, crypto/signer.hpp)
# add_executable(SyncSimulator src/cli/sync_simulator.cpp)
# target_link_libraries(SyncSimulator PRIVATE TurtleClassCore TurtleClassSecurity TurtleClassServer)
# target_include_directories(SyncSimulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()
add_test(NAME TurtleClassTests COMMAND TurtleClassTests)

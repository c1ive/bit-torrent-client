cmake_minimum_required(VERSION 3.24)
project(bit_torrent_client LANGUAGES CXX)

# Suppress Boost policy warning if supported
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies from vcpkg ---
find_package(asio CONFIG REQUIRED)
find_package(ada CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)

# --- Core Library ---
add_library(bt_core STATIC
    src/core/torrent_metadata_loader.cpp
    src/core/bencode_parser.cpp
    src/core/peer_communicator.cpp
    src/core/tracker_communicator.cpp
)

target_include_directories(bt_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bt_core PUBLIC 
    spdlog::spdlog
    ada::ada
    cpr::cpr
    argparse::argparse
)

# --- Main Executable ---
add_executable(bit-torrent-client 
    src/main.cpp 
    src/app/torrent_orchestrator.cpp
    src/app/peer_manager.cpp    
)

target_link_libraries(bit-torrent-client PRIVATE bt_core)

# --- Tests ---
set(TEST_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/doctest)

# Bencode parser tests
add_executable(bt-bencode-tests tests/bencode_parser_tests.cpp)
target_include_directories(bt-bencode-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-bencode-tests PRIVATE bt_core doctest::doctest)

# Metadata loader tests
add_executable(bt-metadata-loader-tests tests/torrent_metadata_loader_tests.cpp)
target_include_directories(bt-metadata-loader-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-metadata-loader-tests PRIVATE bt_core doctest::doctest)

# Connection module tests
add_executable(bt-connection-tests tests/connection_tests.cpp)
target_include_directories(bt-connection-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-connection-tests PRIVATE bt_core doctest::doctest)

# Peer communication tests
add_executable(bt-peer-communication-tests tests/peer_communication_tests.cpp)
target_include_directories(bt-peer-communication-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-peer-communication-tests PRIVATE bt_core doctest::doctest)

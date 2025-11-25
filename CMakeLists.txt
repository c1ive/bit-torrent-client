cmake_minimum_required(VERSION 3.16)
project(bit_torrent_client LANGUAGES CXX)

# Suppress Boost policy warning if supported
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
include(FetchContent)

### spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(spdlog)

### cpr
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.0 # Check for latest version
)
FetchContent_MakeAvailable(cpr)

### argparser
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

# Remove duplicate Boost calls; use single config-mode call with components.
find_package(Boost REQUIRED CONFIG COMPONENTS url)

# Create a static library named 'bt_core'.
add_library(bt_core STATIC
    src/core/torrent_metadata_loader.cpp
    src/core/bencode_parser.cpp
    src/core/tracker_communicator.cpp
)

# Define include directories for the library.
target_include_directories(bt_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies to the core library.
target_link_libraries(bt_core PUBLIC 
    spdlog::spdlog 
    Boost::boost
    Boost::url
    cpr::cpr
)

# --- Main Executable ---
add_executable(bit-torrent-client src/main.cpp src/app/torrent_orchestrator.cpp)

# Link the main app to our core library.
target_link_libraries(bit-torrent-client PRIVATE bt_core argparse)

# --- Tests ---
# Helper for test include paths
set(TEST_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/doctest)

# Bencode parser tests
add_executable(bt-bencode-tests tests/bencode_parser_tests.cpp)
target_include_directories(bt-bencode-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-bencode-tests PRIVATE bt_core)

# Metadata loader tests
add_executable(bt-metadata-loader-tests tests/torrent_metadata_loader_tests.cpp)
target_include_directories(bt-metadata-loader-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-metadata-loader-tests PRIVATE bt_core)

# Connection module tests
add_executable(bt-connection-tests tests/connection_tests.cpp)
target_include_directories(bt-connection-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-connection-tests PRIVATE bt_core)
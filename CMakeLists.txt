cmake_minimum_required(VERSION 3.16)
project(bit_torrent_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.0 # Check for latest version
)
FetchContent_MakeAvailable(cpr)

find_package(Boost REQUIRED)
find_package(Boost REQUIRED COMPONENTS url)

# Create a static library named 'bt_core'.
add_library(bt_core STATIC
    src/core/TorrentMetadataLoader.cpp
    src/core/Handshake.cpp
    src/core/Peer.cpp
    src/core/BencodeParser.cpp
    src/core/Connection.cpp
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
add_executable(bit-torrent-client src/main.cpp)

# Link the main app to our core library.
target_link_libraries(bit-torrent-client PRIVATE bt_core)

# --- Tests ---
# Helper for test include paths
set(TEST_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/doctest)

# Bencode parser tests
add_executable(bt-bencode-tests tests/BencodeParserTests.cpp)
target_include_directories(bt-bencode-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-bencode-tests PRIVATE bt_core)

# Metadata loader tests
add_executable(bt-metadata-loader-tests tests/TorrentMetadataLoaderTests.cpp)
target_include_directories(bt-metadata-loader-tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(bt-metadata-loader-tests PRIVATE bt_core)
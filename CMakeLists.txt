cmake_minimum_required(VERSION 3.16)
project(bit_torrent_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.16.0
)

FetchContent_MakeAvailable(spdlog)

find_package(Boost REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/doctest)

# Main executable
add_executable(bit-torrent-client
    src/main.cpp
    src/core/TorrentMetadataLoader.cpp
    src/core/Handshake.cpp
    src/core/Peer.cpp
    src/core/BencodeParser.cpp
)

# Bencode parser tests
add_executable(bt-bencode-tests
    tests/BencodeParserTests.cpp
    src/core/BencodeParser.cpp
)

# Metadata loader tests
add_executable(bt-metadata-loader-tests
    tests/TorrentMetadataLoaderTests.cpp
    src/core/TorrentMetadataLoader.cpp
    src/core/BencodeParser.cpp
)

target_link_libraries(bit-torrent-client PRIVATE spdlog::spdlog Boost::boost)
target_link_libraries(bt-bencode-tests PRIVATE spdlog::spdlog Boost::boost)
target_link_libraries(bt-metadata-loader-tests PRIVATE spdlog::spdlog Boost::boost)
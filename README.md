# Bit Torrent Client

This repository contains a small BitTorrent client implementation written in C++.
It was created as a personal learning project and is not intended for production use.

## What it is

- A minimal BitTorrent client built for educational purposes.
- Uses `asio`/`boost`-style asynchronous networking and bencode parsing.
- Tracks and downloads pieces from peers, verifies piece hashes, and writes output to disk.

## What it is not

- Not ready for real-world usage.
- Not a hardened or secure BitTorrent implementation.
- Not optimized for performance, stability, or protocol edge cases.

## Requirements

- CMake 3.24 or newer
- Ninja
- A C++20-capable compiler
- `vcpkg` is included as a submodule under `third_party/vcpkg`

## Build

From the repository root:

```bash
cmake --preset debug
cmake --build build/debug
```

If you prefer the release build:

```bash
cmake --preset release
cmake --build build/release
```

## Run

After building, run the client binary from the build folder.

Example:

```bash
./build/debug/bit-torrent-client --torrent ./tests/debian-13.2.0-arm64-netinst.iso.torrent -v
```

Adjust the torrent path as needed.

## Tests

Built test binaries are located under `build/debug/`.

Example test run:

```bash
./build/debug/bt-bencode-tests
./build/debug/bt-metadata-loader-tests
./build/debug/bt-connection-tests
./build/debug/bt-peer-communication-tests
```

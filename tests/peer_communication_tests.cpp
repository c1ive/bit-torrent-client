#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "core/peer_communicator.hpp"
#include "doctest/doctest.h"
#include <string>
#include <vector>

using namespace bt::core;

TEST_CASE("Serialize Handshake") {
    // InfoHash and PeerID are 20 bytes long
    Sha1Hash info_hash = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34,
                          0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78};

    std::string peer_id = "-BT1000-123456789012"; // 20 chars

    HandshakeMsg msg = serializeHandshake(info_hash, peer_id);

    // Check length prefix
    CHECK(msg[0] == 19);

    // Check protocol string "BitTorrent protocol"
    std::string proto_str(reinterpret_cast<char*>(&msg[1]), 19);
    CHECK(proto_str == "BitTorrent protocol");

    // Check reserved bytes (8 bytes of 0)
    for (int i = 20; i < 28; ++i) {
        CHECK(msg[i] == 0);
    }

    // Check info hash
    for (int i = 0; i < 20; ++i) {
        CHECK(msg[28 + i] == info_hash[i]);
    }

    // Check peer id
    for (int i = 0; i < 20; ++i) {
        CHECK(msg[48 + i] == peer_id[i]);
    }
}

TEST_CASE("Verify Handshake") {
    Sha1Hash expected_hash = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34,
                              0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78};

    SUBCASE("Valid Handshake") {
        HandshakeMsg msg;
        msg[0] = 19;
        std::string proto = "BitTorrent protocol";
        std::copy(proto.begin(), proto.end(), msg.begin() + 1);
        // Reserved
        std::fill(msg.begin() + 20, msg.begin() + 28, 0);
        // Info Hash
        std::copy(expected_hash.begin(), expected_hash.end(), msg.begin() + 28);
        // Peer ID (irrelevant for verification logic in our code, but good to have)
        std::fill(msg.begin() + 48, msg.begin() + 68, 'A');

        CHECK(verifyHandshake(msg, expected_hash) == true);
    }

    SUBCASE("Invalid Length") {
        HandshakeMsg msg;
        msg[0] = 18; // Wrong length
        std::string proto = "BitTorrent protocol";
        std::copy(proto.begin(), proto.end(), msg.begin() + 1);
        std::copy(expected_hash.begin(), expected_hash.end(), msg.begin() + 28);

        CHECK(verifyHandshake(msg, expected_hash) == false);
    }

    SUBCASE("Invalid Protocol String") {
        HandshakeMsg msg;
        msg[0] = 19;
        std::string proto = "BitTorrent protocoX"; // Wrong string
        std::copy(proto.begin(), proto.end(), msg.begin() + 1);
        std::copy(expected_hash.begin(), expected_hash.end(), msg.begin() + 28);

        CHECK(verifyHandshake(msg, expected_hash) == false);
    }

    SUBCASE("Invalid Info Hash") {
        HandshakeMsg msg;
        msg[0] = 19;
        std::string proto = "BitTorrent protocol";
        std::copy(proto.begin(), proto.end(), msg.begin() + 1);
        std::fill(msg.begin() + 20, msg.begin() + 28, 0);

        // Wrong hash
        std::copy(expected_hash.begin(), expected_hash.end(), msg.begin() + 28);
        msg[28] = 0xAA; // Corrupt first byte of hash

        CHECK(verifyHandshake(msg, expected_hash) == false);
    }
}

#pragma once
#include <boost/url.hpp>
#include <cpr/cpr.h>
#include <cpr/response.h>
#include <string>
#include <string_view>

#include "torrent_metadata_loader.hpp"

/**
 * bt::core
 *
 * Core BitTorrent tracker communication utilities.
 */

/**
 * TrackerResponse
 *
 * Represents a tracker reply.
 *
 * @var interval   Number of seconds the client should wait before re-announcing.
 * @var peersBlob  Vector of 6-byte peer entries (IPv4: 4 bytes address + 2 bytes port).
 */

/**
 * announceAndGetPeers
 *
 * Perform an announce for the given torrent metadata and return the parsed tracker response.
 *
 * @param metadata  Torrent metadata used to construct the announce.
 * @return          Parsed TrackerResponse containing interval and peer list.
 */

/// Internal helpers (detail namespace) - not part of the public API.
/**
 * generateId
 *
 * Create a random peer id string of the given length.
 *
 * @param length  Desired length of the peer id.
 * @return        Generated peer id.
 */

/**
 * buildTrackerUrl
 *
 * Construct the tracker announce URL using torrent metadata and the peer id.
 *
 * @param metadata  Torrent metadata containing announce URL and parameters.
 * @param peerId    Peer id to include in the announce.
 * @return          Fully formed tracker URL.
 */

/**
 * announceToTracker
 *
 * Perform the HTTP request to the tracker.
 *
 * @param url  Tracker announce URL.
 * @return     HTTP response object from the request.
 */

/**
 * parseTrackerResponse
 *
 * Parse the tracker's response payload into a TrackerResponse structure.
 *
 * @param response  Raw response body from the tracker.
 * @return          Parsed TrackerResponse.
 */

/**
 * toSixByteArrays
 *
 * Convert a compact peers binary blob into a vector of 6-byte arrays (IP+port).
 *
 * @param blob  Binary peers blob as returned by some trackers.
 * @return      Vector of peers represented as 6-byte arrays.
 */

/**
 * debugLogTrackerResponse
 *
 * Emit diagnostic information about a TrackerResponse for debugging purposes.
 *
 * @param r  TrackerResponse to log.
 */
namespace bt::core {

struct TrackerResponse {
    int interval;
    std::vector<std::array<uint8_t, 6>> peersBlob;
};

TrackerResponse announceAndGetPeers(const TorrentMetadata& metadata);

namespace detail {
std::string generateId(int length);
boost::urls::url buildTrackerUrl(const TorrentMetadata& metadata, std::string_view peerId);
cpr::Response announceToTracker(std::string_view url);
TrackerResponse parseTrackerResponse(const std::string_view response);
std::vector<std::array<uint8_t, 6>> toSixByteArrays(std::string_view blob);
void debugLogTrackerResponse(const TrackerResponse& r);
} // namespace detail
} // namespace bt::core
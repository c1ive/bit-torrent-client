// Core module with essentaial networking functionalities.
#include "core/tracker_communicator.hpp"
#include "core/bencode_parser.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <ada.h>
#include <ada/url_aggregator.h>
#include <ada/url_search_params.h>
#include <cpr/response.h>
#include <cstdint>
#include <random>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>

namespace bt::core {

TrackerResponse announceAndGetPeers(const TorrentMetadata& metadata, std::string_view peerId) {
    spdlog::debug("Announcing to tracker: {}", metadata.announce);

    spdlog::debug("Successfully generated a random id: {}", peerId);
    const auto url = detail::buildTrackerUrl(metadata, peerId);
    spdlog::debug("Built the tracker url: {}", url);

    const auto resp = detail::announceToTracker(url);
    const auto trackerR = detail::parseTrackerResponse(resp.text);
    detail::debugLogTrackerResponse(trackerR);

    return trackerR;
}

std::string generateId(int length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::string random_string;
    for (size_t i = 0; i < length; ++i) {
        random_string += characters[distribution(generator)];
    }

    return random_string;
}

namespace detail {
std::string buildTrackerUrl(const TorrentMetadata& metadata, std::string_view peerId) {

    std::string_view infoHashView(reinterpret_cast<const char*>(metadata.infoHash.data()),
                                  metadata.infoHash.size());
    auto finalUrl = ada::parse(metadata.announce);

    ada::url_search_params params;

    params.append("info_hash", infoHashView);
    params.append("peer_id", peerId);
    params.append("uploaded", "0");
    params.append("downloaded", "0");
    params.append("compact", "1");
    params.append("left", std::to_string(metadata.info.fileLength));

    finalUrl->set_search(params.to_string());
    if (finalUrl.has_value()) {
        return std::string{finalUrl.value().get_href()};
    } else {
        throw std::runtime_error{"Failed to build tracker URL"};
    }
}

cpr::Response announceToTracker(std::string_view url) {
    const auto r = cpr::Get(cpr::Url{url});
    if (r.status_code != 200) {
        std::string msg = "Announcing failed: " + r.error.message;
        throw std::runtime_error(msg);
    }

    spdlog::debug("Announce response status code: {}", r.status_code);
    spdlog::debug("Announce response: {}", r.text);

    return r;
}

TrackerResponse parseTrackerResponse(const std::string_view response) {
    const auto& parsedResponse = bencode::parse(response);
    if (!std::holds_alternative<bencode::Dict>(parsedResponse)) {
        spdlog::error("Tracker response root is not a dictionary");
        throw std::runtime_error("Invalid tracker response format");
    }
    const auto& dict = std::get<bencode::Dict>(parsedResponse);

    const auto interval = bencode::extractValueFromDict<int64_t>(dict, "interval");
    const auto& peerBlob = bencode::extractValueFromDict<std::string>(dict, "peers");

    TrackerResponse result;
    result.interval = static_cast<int>(interval);
    result.peersBlob = toSixByteArrays(peerBlob);
    return result;
}

std::vector<std::array<uint8_t, 6>> toSixByteArrays(const std::string_view blob) {
    if (blob.size() % 6 != 0)
        throw std::runtime_error("Peer blob length not multiple of 6");
    std::vector<std::array<uint8_t, 6>> out;
    out.reserve(blob.size() / 6);
    for (size_t i = 0; i < blob.size(); i += 6) {
        std::array<uint8_t, 6> peer{};
        std::copy_n(blob.data() + i, 6, peer.data());
        out.push_back(peer);
    }
    return out;
}

void debugLogTrackerResponse(const TrackerResponse& r) {
    spdlog::info("Tracker response: interval={}, peers={}", r.interval, r.peersBlob.size());
}
} // namespace detail
} // namespace bt::core
#include <boost/url.hpp>
#include <cpr/cpr.h>
#include <string>
#include <string_view>

#include "TorrentMetadataLoader.hpp"
#include "cpr/response.h"

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
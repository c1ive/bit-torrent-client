#include <boost/url.hpp>
#include <cpr/cpr.h>
#include <string>

#include "TorrentMetadataLoader.hpp"
#include "cpr/response.h"

namespace bt::core {
std::string generateId(int length);
boost::urls::url buildTrackerUrl(const TorrentMetadata& metadata, std::string_view peerId);
cpr::Response announce(std::string_view url);
} // namespace bt::core
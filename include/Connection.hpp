#include <boost/asio.hpp>
#include <boost/url.hpp>
// #include <boost/url/url_base.hpp>
#include <string>

#include "TorrentMetadataLoader.hpp"

namespace bt::core {
std::string generateId(int length);
boost::urls::url buildTrackerUrl(const TorrentMetadata& metadata, std::string_view peerId);
} // namespace bt::core
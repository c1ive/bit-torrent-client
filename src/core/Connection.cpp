// Core module with essentaial networking functionalities.
#include <random>
#include <stdexcept>

#include "Connection.hpp"
#include "cpr/response.h"

namespace bt::core {
std::string urlEncode(const std::string_view str) {
    return std::string{};
}
boost::urls::url buildTrackerUrl(const TorrentMetadata& metadata, std::string_view peerId) {
    using namespace boost;

    std::string_view infoHashView(reinterpret_cast<const char*>(metadata.infoHash.data()),
                                  metadata.infoHash.size());

    urls::url finalUrl(metadata.announce);
    finalUrl.set_params({{"info_hash", infoHashView},
                         {"peer_id", peerId},
                         {"uploaded", "0"},
                         {"downloaded", "0"},
                         {"compact", "1"},
                         {"left", std::to_string(metadata.info.fileLength)}});

    finalUrl.normalize();
    return finalUrl;
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

cpr::Response announce(std::string_view url) {
    const auto r = cpr::Get(cpr::Url{url});
    if (r.status_code != 200) {
        throw std::runtime_error{"Announcing failed"};
    }

    return r;
}
} // namespace bt::core
#include <spdlog/spdlog.h>
#include <string>

#include "TorrentMetadataLoader.hpp"
#include "BencodeParser.hpp"

namespace bt::core {
const TorrentMetadata constructTorrentFile( std::string_view& path ) {
    std::string torrentData{};
    try {
        torrentData = detail::loadTorrentFile( path );
    }
    catch ( const std::exception& e ) {
        spdlog::error( "Error loading torrent file: {}", e.what() );
        throw;
    }

    return detail::parseTorrentData( torrentData );
}

namespace detail {
TorrentMetadata parseTorrentData( const std::string& torrentData ) {
    TorrentMetadata metadata;

    // Verify maximal lenght
    constexpr size_t limit = 10 * 1024 * 1024; // 10 MB
    if ( torrentData.size() > limit ) {
        spdlog::error( "Torrent file too large: {} bytes", torrentData.size() );
        throw std::runtime_error( "Torrent file exceeds maximum allowed size" );
    }

    bencode::Value value = bencode::parse( torrentData );

    if ( !std::holds_alternative<bencode::Dict>( value ) ) {
        spdlog::error( "Torrent file root is not a dictionary" );
        throw std::runtime_error( "Invalid torrent file format" );
    }

    const auto& dict = std::get<bencode::Dict>( value );

    // Extract announce URL
    metadata.announce = bencode::extractValueFromDict<std::string>( dict, "announce" );

    // Get the comment and log it
    spdlog::debug( "Parsed torrent metadata: Announce URL = {}", metadata.announce );

    return metadata;
}
std::string loadTorrentFile(std::string_view& path) {
    spdlog::info( "Loading torrent file from path: {}", path );
    
    // get a file descriptor
    FILE* file = fopen( std::string( path ).c_str(), "rb" );
    if ( !file ) {
        throw std::runtime_error( "Failed to open torrent file" );
    }

    // seek to end to get file size
    fseek( file, 0, SEEK_END );
    long fileSize = ftell( file );
    fseek( file, 0, SEEK_SET );

    // read file contents
    std::vector<uint8_t> fileData( fileSize );
    size_t bytesRead = fread( fileData.data(), 1, fileSize, file );
    if ( bytesRead != static_cast<size_t>( fileSize ) ) {
        spdlog::error( "Failed to read entire torrent file: {}", path );
        fclose( file );
        throw std::runtime_error( "Failed to read entire torrent file" );
    }
    fclose( file );

    return std::string( reinterpret_cast<const char*>( fileData.data() ), fileData.size() );
}
}
}
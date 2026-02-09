#include "app/progress_tracker.hpp"
#include <cstdio>
#include <iostream> // for std::flush
#include <string>

namespace bt {

ProgressTracker::ProgressTracker(int totalPieces, int frequency)
    : _totalPieces(totalPieces), _frequency(frequency) {

    std::printf("Bittorrent Client - V0.0.1\n");
    std::printf("Download Progress:\n");
    _startTime = std::chrono::high_resolution_clock::now();
    _thread = std::thread([this]() {
        while (_keepRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(_frequency));
            _logProgress();
        }
    });
}

ProgressTracker::~ProgressTracker() {
    _keepRunning = false;
    if (_thread.joinable()) {
        _thread.join();
    }

    _logProgress();
    std::cout << std::endl;
}

void ProgressTracker::notifyProgress() {
    ++_finishedPieces;
}

void ProgressTracker::_logProgress() {
    double progress = static_cast<double>(_finishedPieces) / _totalPieces;
    double percent = progress * 100.0;

    if (percent > 100.0)
        percent = 100.0;

    int amountHash = static_cast<int>(percent);
    int amountSpace = 100 - amountHash;

    if (amountSpace < 0)
        amountSpace = 0;

    std::string hashtags(amountHash, '#');
    std::string spaces(amountSpace, ' ');

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSeconds = now - _startTime;

    std::printf("\r[%.2fs] [%.2f%%] [%s%s]", elapsedSeconds.count(), percent, hashtags.c_str(),
                spaces.c_str());
    std::fflush(stdout);
}

} // namespace bt
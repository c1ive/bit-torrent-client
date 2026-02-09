#include "app/progress_tracker.hpp"
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>

namespace bt {

ProgressTracker::ProgressTracker(int totalPieces, int frequency)
    : _totalPieces(totalPieces), _frequency(frequency) {

    std::time_t now_time = std::time(nullptr);
    char time_str[20];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));

    std::printf("\033[?25l");
    std::printf("\033[1;34m==========================================================\033[0m\n");
    std::printf("\033[1;36m BIT-TORRENT CLIENT \033[0m| \033[1;32mVersion 0.0.1-alpha\033[0m\n");
    std::printf("\033[1;37m Created by: \033[1;33mJonas Sasowski\033[0m\n");
    std::printf("\033[1;37m Session Start: \033[0m%s\n", time_str);
    std::printf("\033[1;37m Total Workload: \033[1;32m%d Pieces\033[0m\n", _totalPieces);
    std::printf("\033[1;34m==========================================================\033[0m\n\n");
    std::printf("\033[1mCurrent Task Status:\033[0m\n");

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
    std::printf("\033[1;32m\n[SUCCESS] Download session finalized.\033[0m\n");
    std::printf("\033[?25h");
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

    std::printf("\r\033[1;33m[%.2fs]\033[0m \033[1;36m[%.2f%%]\033[0m "
                "[\033[1;32m%s\033[0m\033[2m%s\033[0m]\033[K",
                elapsedSeconds.count(), percent, hashtags.c_str(), spaces.c_str());
    std::fflush(stdout);
}

} // namespace bt
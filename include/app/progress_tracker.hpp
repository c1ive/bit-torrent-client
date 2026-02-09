#pragma once
#include <atomic>
#include <chrono>
#include <thread>

namespace bt {
class ProgressTracker {
public:
    ProgressTracker(int totalPieces, int frequency);
    ~ProgressTracker();
    void notifyProgress();

private:
    void _logProgress();

    int _totalPieces;
    int _frequency;

    std::atomic<int> _finishedPieces{0};
    std::atomic<bool> _keepRunning{true};

    std::thread _thread;
    std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
};
} // namespace bt
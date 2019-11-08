#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <chrono>

using ProgressMonitor = std::function<void(const std::string& jobDescription, float progress, std::chrono::high_resolution_clock::duration, bool jobDone)>;

class ProgressTracker {
public:
    explicit ProgressTracker(ProgressMonitor monitor);
    void startNewJob(const std::string& jobDescription, int tasks);
    void signalTaskFinished();

private:
    std::string jobDescription;
    int totalTasks;
    int tasksCompleted;
    std::chrono::high_resolution_clock::time_point jobStart;
    std::mutex mutex;
    ProgressMonitor monitor;
};
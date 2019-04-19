#pragma once

#include <functional>
#include <mutex>

using ProgressMonitor = std::function<void(float progress)>;

class ProgressTracker {
public:
    ProgressTracker(ProgressMonitor monitor, int jobs);
    void signalJobFinished();

private:
    int totalJobs;
    int jobsCompleted;
    std::mutex mutex;
    ProgressMonitor monitor;
};
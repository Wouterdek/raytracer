#pragma once

#include <functional>
#include <mutex>
#include <string>

using ProgressMonitor = std::function<void(const std::string& jobDescription, float progress, bool jobDone)>;

class ProgressTracker {
public:
    explicit ProgressTracker(ProgressMonitor monitor);
    void startNewJob(const std::string& jobDescription, int tasks);
    void signalTaskFinished();

private:
    std::string jobDescription;
    int totalTasks;
    int tasksCompleted;
    std::mutex mutex;
    ProgressMonitor monitor;
};
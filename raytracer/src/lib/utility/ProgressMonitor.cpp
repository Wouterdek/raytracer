#include "ProgressMonitor.h"

ProgressTracker::ProgressTracker(ProgressMonitor monitor)
    : monitor(std::move(monitor)), jobDescription(""), totalTasks(0), tasksCompleted(0)
{ }

void ProgressTracker::startNewJob(const std::string& newJobDescription, int tasks)
{
    std::lock_guard g(mutex);
    this->jobDescription = newJobDescription;
    this->totalTasks = tasks;
    this->tasksCompleted = 0;
    auto duration = std::chrono::high_resolution_clock::duration{};
    this->monitor(this->jobDescription, static_cast<float>(this->tasksCompleted) / totalTasks, duration, this->tasksCompleted == totalTasks);
    this->jobStart = std::chrono::high_resolution_clock::now();
}

void ProgressTracker::signalTaskFinished()
{
    std::lock_guard g(mutex);
    auto duration = std::chrono::high_resolution_clock::now() - this->jobStart;
    ++this->tasksCompleted;
    this->monitor(this->jobDescription, static_cast<float>(this->tasksCompleted) / totalTasks, duration, this->tasksCompleted == totalTasks);
}

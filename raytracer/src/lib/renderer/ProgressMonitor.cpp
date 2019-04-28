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
    this->monitor(this->jobDescription, static_cast<float>(++this->tasksCompleted) / totalTasks);
}

void ProgressTracker::signalTaskFinished()
{
    std::lock_guard g(mutex);
    this->monitor(this->jobDescription, static_cast<float>(++this->tasksCompleted) / totalTasks);
}

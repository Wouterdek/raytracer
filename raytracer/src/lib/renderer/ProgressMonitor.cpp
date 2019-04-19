#include "ProgressMonitor.h"

ProgressTracker::ProgressTracker(ProgressMonitor monitor, int jobs)
    : monitor(std::move(monitor)), totalJobs(jobs), jobsCompleted(0)
{ }

void ProgressTracker::signalJobFinished()
{
    std::lock_guard g(mutex);
    this->monitor(static_cast<float>(++this->jobsCompleted) / totalJobs);
}

#include "Task.h"

#ifdef NO_TBB
#include <thread>

class SuperTask : public Task
{
public:
    std::vector<std::unique_ptr<Task>>::iterator begin;
    std::vector<std::unique_ptr<Task>>::iterator end;

    SuperTask(const std::vector<std::unique_ptr<Task>>::iterator& begin,
              const std::vector<std::unique_ptr<Task>>::iterator& end) : begin(begin), end(end) {}

    void execute() override
    {
        for(auto it = begin; it < end; ++it)
        {
            (*it)->execute();
        }
    }
};

void Task::runTasks(std::vector<std::unique_ptr<Task>>& tasks)
{
    using size_type = std::vector<std::unique_ptr<Task>>::size_type;

    auto threadCount = std::thread::hardware_concurrency();
    auto batchSize = std::max(size_type(1), tasks.size() / threadCount);

    std::vector<SuperTask> superTasks;
    for(size_type i = 0; i < tasks.size(); i += batchSize)
    {
        auto endIdx = std::min(i+batchSize, tasks.size());
        bool isLastTask = endIdx+batchSize > tasks.size();
        if(isLastTask)
        {
            endIdx = tasks.size();
        }
        superTasks.emplace_back(tasks.begin()+i, tasks.begin()+endIdx);
    }

    std::vector<std::thread> threads;
    threads.reserve(superTasks.size());
    for(auto& superTask : superTasks)
    {
        threads.emplace_back(&SuperTask::execute, &superTask);
    }
    for(auto& thread : threads)
    {
        thread.join();
    }
}
#else
#include <tbb/tbb.h>

void Task::runTasks(std::vector<std::unique_ptr<Task>>& tasks)
{
    tbb::parallel_for_each(tasks.begin(), tasks.end(), [](const std::unique_ptr<Task>& curTask)
    {
        curTask->execute();
    });
}
#endif
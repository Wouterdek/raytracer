#pragma once

#include <vector>
#include <memory>

class Task
{
public:
    virtual ~Task() = default;
    virtual void execute() = 0;

    static void runTasks(std::vector<std::unique_ptr<Task>>& tasks);
};
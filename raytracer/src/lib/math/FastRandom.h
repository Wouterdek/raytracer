#pragma once

#include <random>

namespace {
    //thread_local std::random_device randDev();
    thread_local std::mt19937 randDev;
}

class Rand
{
public:
    static float unit()
    {
        return std::uniform_real_distribution<float>(0, 1)(randDev);
    }

    static float floatInRange(float a, float b)
    {
        return std::uniform_real_distribution<float>(a, b)(randDev);
    }

    static float floatInRange(float maxValue)
    {
        return std::uniform_real_distribution<float>(0, maxValue)(randDev);
    }

    static int intInRange(int a, int b)
    {
        return std::uniform_int_distribution<int>(a, b)(randDev);
    }

    static int intInRange(int maxValue)
    {
        return std::uniform_int_distribution<int>(0, maxValue)(randDev);
    }

    static float sampleStdNormalDist()
    {
        return std::normal_distribution<float>(0, 1)(randDev);
    }
};

#pragma once

#include <random>
#include "pcg_random.hpp"

class RandDeviceSource
{
public:
    RandDeviceSource()
    {
        // Make a random number engine, seeded with a real random value, if available
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        randDev = pcg32_fast(seed_source);
    }

    //std::random_device randDev;
    //std::mt19937 randDev;
    pcg32_fast randDev;
};

namespace {
    thread_local RandDeviceSource randDevSrc;
}

class Rand
{
public:
    static float unit()
    {
        return std::uniform_real_distribution<float>(0, 1)(randDevSrc.randDev);
    }

    static float floatInRange(float a, float b)
    {
        return std::uniform_real_distribution<float>(a, b)(randDevSrc.randDev);
    }

    static float floatInRange(float maxValue)
    {
        return std::uniform_real_distribution<float>(0, maxValue)(randDevSrc.randDev);
    }

    static int intInRange(int a, int b)
    {
        return std::uniform_int_distribution<int>(a, b)(randDevSrc.randDev);
    }

    static int intInRange(int maxValue)
    {
        return std::uniform_int_distribution<int>(0, maxValue)(randDevSrc.randDev);
    }

    static float sampleStdNormalDist()
    {
        return std::normal_distribution<float>(0, 1)(randDevSrc.randDev);
    }
};

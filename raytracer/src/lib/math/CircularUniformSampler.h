#pragma once

#include "math/Vector2.h"
#include "math/Constants.h"
#include <random>

namespace {
    thread_local std::random_device randDev;
    std::uniform_real_distribution<float> randAngle(0,2*PI);
    std::uniform_real_distribution<float> randUniform(0,1);
};

inline Vector2 sampleUniformCircle(float radius)
{
    float angle = randAngle(randDev);
    float dist = randUniform(randDev) * radius;
    float xOffset = std::cos(angle)*dist;
    float yOffset = std::sin(angle)*dist;
    return Vector2(xOffset, yOffset);
}
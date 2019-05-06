#pragma once

#include "math/Vector2.h"
#include "math/Constants.h"
#include "Vector3.h"
#include <random>

namespace {
    thread_local std::random_device randDev;
    std::normal_distribution<float> randNorm(0, 1);
    std::uniform_real_distribution<float> randAngle(0,2*PI);
    std::uniform_real_distribution<float> randUnit(0,1);
};

inline Vector3 sampleUniformSphere(float radius)
{
    float x = randNorm(randDev);
    float y = randNorm(randDev);
    float z = randNorm(randDev);
    float len = x*x + y*y + z*z;
    return Vector3(radius*x/len, radius*y/len, radius*z/len);
}

inline Vector3 sampleUniformHemisphere(float radius)
{
    float x = randNorm(randDev);
    float y = randNorm(randDev);
    float z = abs(randNorm(randDev));
    float len = x*x + y*y + z*z;
    return Vector3(radius*x/len, radius*y/len, radius*z/len);
}

inline Vector2 sampleUniformCircle(float radius)
{
    float angle = randAngle(randDev);
    float dist = randUnit(randDev) * radius;
    float xOffset = std::cos(angle)*dist;
    float yOffset = std::sin(angle)*dist;
    return Vector2(xOffset, yOffset);
}

inline Vector3 sampleUniformTriangle(const Vector3& a, const Vector3& b, const Vector3& c)
{
    auto u = 1.0 - sqrt(randUnit(randDev));
    auto v = (1.0 - u) * randUnit(randDev);

    return a + u*(c - a) + v*(b - a);
}
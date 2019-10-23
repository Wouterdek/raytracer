#pragma once

#include <cmath>

#include "math/Vector2.h"
#include "math/Constants.h"
#include "math/FastRandom.h"
#include "Vector3.h"

inline Vector3 sampleUniformSphere(float radius)
{
    float x = Rand::sampleStdNormalDist();
    float y = Rand::sampleStdNormalDist();
    float z = Rand::sampleStdNormalDist();
    float len = x*x + y*y + z*z;
    return Vector3(radius*x/len, radius*y/len, radius*z/len);
}

inline Vector3 sampleUniformHemisphere(float radius)
{
    float x = Rand::sampleStdNormalDist();
    float y = Rand::sampleStdNormalDist();
    float z = abs(Rand::sampleStdNormalDist());
    float len = x*x + y*y + z*z;
    return Vector3(radius*x/len, radius*y/len, radius*z/len);
}

inline Vector2 sampleUniformCircle(float radius)
{
    float angle = Rand::floatInRange(2.0f*PI);
    float dist = Rand::floatInRange(radius);
    float xOffset = std::cos(angle)*dist;
    float yOffset = std::sin(angle)*dist;
    return Vector2(xOffset, yOffset);
}

inline Vector3 sampleUniformTriangle(const Vector3& a, const Vector3& b, const Vector3& c)
{
    float u = 1.0f - std::sqrt(Rand::unit());
    auto v = Rand::floatInRange(1.0f - u);

    return a + u*(c - a) + v*(b - a);
}

// sample square between (0, 0) and (1, 1)
inline Vector2 sampleStratifiedSquare(int level, int sampleI)
{
    if(level == 1)
    {
        return Vector2(0.5f, 0.5f);
    }
    else
    {
        float lvl = level;
        const auto x = (static_cast<float>(sampleI % level) / level) + (Rand::unit() / lvl);
        const auto y = (static_cast<float>(std::floor(sampleI / level)) / lvl) + (Rand::unit() / lvl);
        return Vector2(x, y);
    }
}
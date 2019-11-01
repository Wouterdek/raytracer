#pragma once

#include <cmath>

#include "math/Vector2.h"
#include "math/Constants.h"
#include "math/FastRandom.h"
#include "Vector3.h"

// sample square between (0, 0) and (1, 1)
inline Vector2 sampleUniformStratifiedSquare(int level, int sampleI)
{
    if(level == 1)
    {
        return Vector2(0.5f, 0.5f);
    }
    else
    {
        auto lvl = static_cast<float>(level);
        const auto x = (static_cast<float>(sampleI % level) / lvl) + (Rand::unit() / lvl);
        const auto y = (static_cast<float>(std::floor(sampleI / level)) / lvl) + (Rand::unit() / lvl);
        return Vector2(x, y);
    }
}

inline Vector3 sampleUniformSphere(float radius)
{
    float x = Rand::sampleStdNormalDist();
    float y = Rand::sampleStdNormalDist();
    float z = Rand::sampleStdNormalDist();
    float len = x*x + y*y + z*z;
    return Vector3(radius*x/len, radius*y/len, radius*z/len);
}

inline Vector3 mapSampleToCosineWeightedHemisphere(float r1, float r2, float radius, float exponent = 1.0f)
{
    auto cos_theta = std::pow(r1, 1.0f/(exponent+1.0f)); //todo: can be simplified by calculating sin_theta first (assuming e = 1)
    auto sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
    auto phi = 2.0f * PI * r2;
    auto cos_phi = std::cos(phi);
    auto sin_phi = std::sin(phi);
    return Vector3(radius * sin_theta * cos_phi, radius * sin_theta * sin_phi, radius * cos_theta);
}

inline Vector3 sampleStratifiedCosineWeightedHemisphere(int level, int sampleI, float radius, float exponent = 1.0f)
{
    auto sample = sampleUniformStratifiedSquare(level, sampleI);
    return mapSampleToCosineWeightedHemisphere(sample.x(), sample.y(), radius, exponent);
}

inline Vector3 sampleUniformHemisphere(float radius)
{
    return mapSampleToCosineWeightedHemisphere(Rand::unit(), Rand::unit(), radius, 0.0f);
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

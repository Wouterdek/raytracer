#pragma once

#include <cmath>

#include "math/Vector2.h"
#include "math/Constants.h"
#include "math/FastRandom.h"
#include "math/OrthonormalBasis.h"
#include "math/Transformation.h"
#include "Vector3.h"

// sample square between (0, 0) and (1, 1)
inline Vector2 sampleUniformStratifiedSquare(int level, float sampleI)
{
    if(level < 4)
    {
        return Vector2(Rand::unit(), Rand::unit());
    }
    int binsPerAxis = std::sqrt(level);
    float binX = std::fmod(sampleI, binsPerAxis);
    float binY = std::floor(std::fmod(sampleI, level) / binsPerAxis);
    return Vector2(
            (binX/binsPerAxis) + (Rand::unit() / binsPerAxis),
            (binY/binsPerAxis) + (Rand::unit() / binsPerAxis)
    );
}

inline Vector3 sampleUniformStratifiedCube(float level, float sampleI)
{
    float binsPerAxis = std::pow(level, 1.0f/3.0f);
    float oneOverBinsPerAxis = 1.0f / binsPerAxis;
    float binX = std::fmod(sampleI, binsPerAxis);
    float binY = std::floor(std::fmod(sampleI, binsPerAxis*binsPerAxis) * oneOverBinsPerAxis);
    float binZ = std::floor(std::fmod(sampleI, level) * (oneOverBinsPerAxis * oneOverBinsPerAxis));
    return Vector3(
        (binX * oneOverBinsPerAxis) + (Rand::unit() * oneOverBinsPerAxis),
        (binY * oneOverBinsPerAxis) + (Rand::unit() * oneOverBinsPerAxis),
        (binZ * oneOverBinsPerAxis) + (Rand::unit() * oneOverBinsPerAxis)
    );
}

inline Vector3 sampleUniformSphere(float radius)
{
    float x = Rand::sampleStdNormalDist();
    float y = Rand::sampleStdNormalDist();
    float z = Rand::sampleStdNormalDist();
    float len = std::sqrt(x*x + y*y + z*z);
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
    assert(sample.x() >= 0 && sample.x() <= 1);
    assert(sample.y() >= 0 && sample.y() <= 1);
    auto val = mapSampleToCosineWeightedHemisphere(sample.x(), sample.y(), radius, exponent);
    assert(!val.hasNaN());
    return val;
}

inline Vector3 sampleUniformHemisphere(float radius)
{
    return mapSampleToCosineWeightedHemisphere(Rand::unit(), Rand::unit(), radius, 0.0f);
}

inline Vector3 sampleUniformSteradianSphere(const Vector3& center, float angle)
{
    float alpha = (Rand::unit() - 0.5f) * 2.0f * angle;
    float beta = (Rand::unit() - 0.5f) * 2.0f * angle;
    auto transformedResult = Transformation::rotateX(alpha).append(Transformation::rotateY(beta)).transform(Vector3(0.0f, 0.0f, 1.0f));

    OrthonormalBasis basis(center);
    Eigen::Matrix3f transform{};
    transform.col(0) = basis.getU();
    transform.col(1) = basis.getV();
    transform.col(2) = basis.getW();
    return transform * transformedResult;
}

inline Vector2 sampleUniformCircle(float radius, float rand1, float rand2, float rand3)
{
    //https://stackoverflow.com/a/5838055/915418

    float angle = rand1*2.0f*M_PI;
    float dist = (rand2*radius) + (rand3*radius);
    if(dist > radius)
    {
        dist = (2.0f * radius) - dist;
    }
    float xOffset = std::cos(angle) * dist;
    float yOffset = std::sin(angle) * dist;
    return Vector2(xOffset, yOffset);
}

inline Vector2 sampleUniformCircle(float radius)
{
    return sampleUniformCircle(radius, Rand::unit(), Rand::unit(), Rand::unit());
}

inline Vector2 sampleUniformStratifiedCircle(float radius, int level, int sampleI)
{
    Vector3 cube = sampleUniformStratifiedCube(level, sampleI);
    return sampleUniformCircle(radius, cube.x(), cube.y(), cube.z());
}

inline Vector3 sampleUniformTriangle(const Vector3& a, const Vector3& b, const Vector3& c)
{
    float u = 1.0f - std::sqrt(Rand::unit());
    auto v = Rand::floatInRange(1.0f - u);

    return a + u*(c - a) + v*(b - a);
}

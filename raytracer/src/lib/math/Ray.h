#pragma once

#include <string>
#include <sstream>
#include <array>
#include "Vector3.h"

class Ray {
public:
	Ray(Point origin, Vector3 dir) : origin(std::move(origin))
    {
        this->direction = std::move(dir);
    }

    Ray() : Ray(Point(), Vector3()) {}

	const Point& getOrigin() const noexcept
    {
		return this->origin;
	}

	const Vector3& getDirection() const noexcept
	{
		return this->direction;
	}

private:
	Point origin;
	Vector3 direction;
};

std::ostream& operator<<(std::ostream& in, const Ray& ray);


using RBSize_t = unsigned char;
constexpr RBSize_t RayBundleSize = 32;
using RayBundle = std::array<Ray, RayBundleSize>;
using RayBundlePermutation = std::array<RBSize_t, RayBundleSize>;
template<typename TRayHitInfo>
using HitBundle = std::array<std::optional<TRayHitInfo>, RayBundleSize>;

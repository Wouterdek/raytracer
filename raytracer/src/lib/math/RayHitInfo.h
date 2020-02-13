#pragma once

#include "Ray.h"
#include "Vector3.h"
#include "Vector2.h"
#include "utility/ICloneable.h"
#include <memory>
#include <stdint.h>

class IRayHitAnnex : public ICloneable<IRayHitAnnex>
{ };

class RayHitInfo
{
public:
	RayHitInfo(Ray ray, double t, Vector3 normal, Vector2 texCoord, Vector3 tangent, uint32_t triangleIndex)
		: ray(std::move(ray)), t(t), normal(std::move(normal)), texCoord(std::move(texCoord)), tangent(std::move(tangent)), triangleIndex(triangleIndex)
	{
		assert(!std::isnan(t));
	}

    RayHitInfo(Ray ray, double t, Vector3 normal, Vector2 texCoord, Vector3 tangent)
            : RayHitInfo(ray, t, normal, texCoord, tangent, UINT32_MAX)
    {}

	RayHitInfo(const RayHitInfo& b)
	    : ray(b.ray), t(b.t), normal(b.normal), texCoord(b.texCoord), tangent(b.tangent), triangleIndex(b.triangleIndex)
    {}
	RayHitInfo(RayHitInfo&& b) noexcept
	    : ray(std::move(b.ray)), t(b.t), normal(std::move(b.normal)), texCoord(std::move(b.texCoord)), tangent(std::move(b.tangent)), triangleIndex(b.triangleIndex)
    {}
	RayHitInfo& operator=(const RayHitInfo& b)
    {
        *this = RayHitInfo(b);
	    return *this;
    }
	RayHitInfo& operator=(RayHitInfo&& b) noexcept
    {
	    this->ray = std::move(b.ray);
	    this->t = b.t;
	    this->normal = b.normal;
	    this->texCoord = b.texCoord;
	    this->tangent = b.tangent;
	    this->triangleIndex = b.triangleIndex;
	    return *this;
	}

	Point getHitpoint() const noexcept
    {
        return ray.getOrigin() + (ray.getDirection() * t);
    }

	Ray ray;
	double t;
	Vector3 normal;
    Vector3 tangent;
	Vector2 texCoord;
    uint32_t triangleIndex;
};

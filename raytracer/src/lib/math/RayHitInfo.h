#pragma once

#include "Ray.h"
#include "Vector3.h"
#include "Vector2.h"
#include "utility/ICloneable.h"
#include <memory>

class IRayHitAnnex : public ICloneable<IRayHitAnnex>
{ };

class RayHitInfo
{
public:
	RayHitInfo(Ray ray, double t, Vector3 normal, Vector2 texCoord)
		: ray(std::move(ray)), t(t), normal(std::move(normal)), texCoord(std::move(texCoord)), annex(nullptr)
	{
		assert(!std::isnan(t));
	}

    RayHitInfo(Ray ray, double t, Vector3 normal, Vector2 texCoord, std::unique_ptr<IRayHitAnnex> annex)
            : RayHitInfo(ray, t, normal, texCoord)
    {
	    this->annex = std::move(annex);
    }

	RayHitInfo(const RayHitInfo& b)
	    : ray(b.ray), t(b.t), normal(b.normal), texCoord(b.texCoord), annex(nullptr)
    {
	    if(b.annex != nullptr){
            annex = b.annex->clone();
	    }
    }
	RayHitInfo(RayHitInfo&& b) noexcept
	    : ray(std::move(b.ray)), t(b.t), normal(std::move(b.normal)), texCoord(std::move(b.texCoord)), annex(std::move(b.annex))
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
	    this->normal = std::move(b.normal);
	    this->texCoord = b.texCoord;
	    this->annex = std::move(b.annex);
	    return *this;
	}

	Point getHitpoint() const;

	Ray ray;
	double t;
	Vector3 normal;
	Vector2 texCoord;
	std::unique_ptr<IRayHitAnnex> annex;
};

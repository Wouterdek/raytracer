#pragma once
#include "IShape.h"

class Plane : public IShape
{
public:
	Plane();

	Point getCentroid() const override;
	AABB getAABB() const override;
	std::optional<RayHitInfo> intersect(const Ray& ray) const override;
};
#pragma once

#include "IShape.h"
#include "../math/Transformation.h"

class Sphere : public IShape
{
public:
	Sphere();

	Point getCentroid() const override;
	Box getAABB() const override;
	std::optional<RayHitInfo> intersect(const Ray& ray) const override;
};
#pragma once
#include "IShape.h"

class Plane : public IShape
{
public:
	Plane();
	std::optional<RayHitInfo> intersect(const Ray& ray, const Transformation& transform) const override;
};
#pragma once

#include "IShape.h"
#include "../math/Transformation.h"

class Sphere : public IShape
{
public:
	std::optional<RayHitInfo> intersect(const Ray& ray, const Transformation& transform) const override;
};
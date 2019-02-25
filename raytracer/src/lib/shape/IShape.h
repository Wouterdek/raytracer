#pragma once

#include "../math/Ray.h"
#include "../math/Transformation.h"
#include <optional>
#include "../math/RayHitInfo.h"

class IShape
{
public:
	virtual ~IShape(){};
	virtual std::optional<RayHitInfo> intersect(const Ray& ray, const Transformation& transform) const = 0;
};

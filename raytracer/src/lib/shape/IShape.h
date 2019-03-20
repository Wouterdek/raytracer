#pragma once

#include "math/Ray.h"
#include "math/Transformation.h"
#include <optional>
#include "math/RayHitInfo.h"
#include "AABB.h"

class IShape
{
public:
	virtual ~IShape() = default;

	virtual Point getCentroid() const = 0;
	virtual AABB getAABB() const = 0;
	virtual std::optional<RayHitInfo> intersect(const Ray& ray) const = 0;
};

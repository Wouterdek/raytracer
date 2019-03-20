#pragma once

#include "IShape.h"
#include "AABB.h"

class Box : public IShape, public AABB
{
public:
	Box();
	Box(Point start, Point end);

	Point getCentroid() const override;
	AABB getAABB() const override;
	std::optional<RayHitInfo> intersect(const Ray& ray) const;

//private:
	Vector3 getNormal(const Ray& ray, double t) const;
};

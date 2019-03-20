#pragma once
#include "math/Vector3.h"
#include "math/RayHitInfo.h"
#include "math/Transformation.h"

class AABB
{
public:
	AABB();
	AABB(Point start, Point end);

	std::array<Point, 8> getCorners() const;
	AABB getAABBOfTransformed(const Transformation& transform) const;
	double getSurfaceArea() const;
	bool intersects(const Ray& ray) const;
	AABB merge(const AABB& b) const;
	
	const Point& getStart() const
	{
		return start;
	}

	const Point& getEnd() const
	{
		return end;
	}

protected:
	Point start;
	Point end;
};

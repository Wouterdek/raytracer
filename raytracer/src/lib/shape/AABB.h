#pragma once

#include <math/Axis.h>
#include "math/Vector3.h"
#include "math/RayHitInfo.h"
#include "math/Transformation.h"

class AABB
{
public:
    static const AABB MAX_RANGE;

	AABB();
	AABB(Point start, Point end);

	std::array<Point, 8> getCorners() const;
	AABB getAABBOfTransformed(const Transformation& transform) const;
	double getSurfaceArea() const;
	bool intersects(const Ray& ray) const;
	float getIntersection(const Ray& ray) const;
	AABB merge(const AABB& b) const;
	
	const Point& getStart() const
	{
		return start;
	}

	const Point& getEnd() const
	{
		return end;
	}

    std::array<AABB, 2> split(Axis axis, float coordinate) const
    {
        auto axisIdx = static_cast<int>(axis);
        auto e1 = getEnd();
        e1[axisIdx] = coordinate;
        auto s2 = getStart();
        s2[axisIdx] = coordinate;
        return {
            AABB(getStart(), e1),
            AABB(s2, getEnd())
        };
	}

    Point projectPointOntoAABB(const Point& point) const
    {
        Point projection;
        for(auto i = 0; i < 3; i++)
        {
            projection[i] = std::clamp(point[i], getStart()[i], getEnd()[i]);
        }
        return projection;
    }

protected:
	Point start;
	Point end;
};

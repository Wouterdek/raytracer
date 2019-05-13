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
    bool getIntersections(const Ray& ray, float& t1, float& t2) const;
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

inline bool AABB::getIntersections(const Ray& ray, float& t0, float& t1) const
{
    const Point& o = ray.getOrigin();
    const Vector3& d = ray.getDirection();
    const Point& s = this->start;
    const Point& e = this->end;

    float tx_min, ty_min, tz_min;
    float tx_max, ty_max, tz_max;
    float a = 1.0f / d.x();
    if (a >= 0)
    {
        tx_min = (s.x() - o.x()) * a;
        tx_max = (e.x() - o.x()) * a;
    }
    else
    {
        tx_min = (e.x() - o.x()) * a;
        tx_max = (s.x() - o.x()) * a;
    }

    float b = 1.0f / d.y();
    if (b >= 0)
    {
        ty_min = (s.y() - o.y()) * b;
        ty_max = (e.y() - o.y()) * b;
    }
    else
    {
        ty_min = (e.y() - o.y()) * b;
        ty_max = (s.y() - o.y()) * b;
    }

    float c = 1.0f / d.z();
    if (c >= 0)
    {
        tz_min = (s.z() - o.z()) * c;
        tz_max = (e.z() - o.z()) * c;
    }
    else
    {
        tz_min = (e.z() - o.z()) * c;
        tz_max = (s.z() - o.z()) * c;
    }

    t0 = std::max(tx_min, ty_min);
    t0 = std::max(t0, tz_min);

    t1 = std::min(tx_max, ty_max);
    t1 = std::min(t1, tz_max);

    bool isIntersecting = t0 <= t1;
    t0 = std::max(0.0f, t0);
    return isIntersecting;
}

inline float AABB::getIntersection(const Ray& ray) const
{
    float t0, t1;
    if(!getIntersections(ray, t0, t1))
    {
        return -1.0f;
    }
    return t0;
}
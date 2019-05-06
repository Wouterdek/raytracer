#include "AABB.h"
#include <limits>

#define FLOAT_MAX std::numeric_limits<float>::max()
const AABB AABB::MAX_RANGE(Point(-FLOAT_MAX, -FLOAT_MAX, -FLOAT_MAX), Point(FLOAT_MAX, FLOAT_MAX, FLOAT_MAX));

AABB::AABB()
	: start(Point(0, 0, 0)), end(Point(0, 0, 0))
{
}

AABB::AABB(Point start, Point end)
	: start(std::move(start)), end(std::move(end))
{
	assert(!hasNaN(start));
	assert(!hasNaN(end));
}

std::array<Point, 8> AABB::getCorners() const
{
	return {
		start,
		Point(start.x(), start.y(), end.z()),
		Point(start.x(), end.y(), start.z()),
		Point(end.x(), start.y(), start.z()),
		Point(start.x(), end.y(), end.z()),
		Point(end.x(), start.y(), end.z()),
		Point(end.x(), end.y(), start.z()),
		end
	};
}

AABB AABB::getAABBOfTransformed(const Transformation & transform) const
{
	auto points = getCorners();
	for (int i = 0; i < points.size(); i++)
	{
		points[i] = transform.transform(points[i]);
	}
	auto [minXPoint, maxXPoint] = std::minmax_element(points.begin(), points.end(), [](const auto & p1, const auto & p2) { return p1.x() < p2.x(); });
	auto [minYPoint, maxYPoint] = std::minmax_element(points.begin(), points.end(), [](const auto & p1, const auto & p2) { return p1.y() < p2.y(); });
	auto [minZPoint, maxZPoint] = std::minmax_element(points.begin(), points.end(), [](const auto & p1, const auto & p2) { return p1.z() < p2.z(); });
	Point newStart(minXPoint->x(), minYPoint->y(), minZPoint->z());
	Point newEnd(maxXPoint->x(), maxYPoint->y(), maxZPoint->z());
	return AABB(newStart, newEnd);
}

bool AABB::intersects(const Ray & ray) const
{
	return this->getIntersection(ray) != -1;
}

float AABB::getIntersection(const Ray& ray) const
{
	Vector3 o = ray.getOrigin();
	Vector3 d = ray.getDirection();
	Vector3 s = this->start;
	Vector3 e = this->end;

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

	float t0 = std::max(tx_min, ty_min);
	t0 = std::max(t0, tz_min);

	float t1 = std::min(tx_max, ty_max);
	t1 = std::min(t1, tz_max);

	if (t0 > t1)
	{
		return -1;
	}
	else
	{
		return std::max(0.0f, t0);
	}
}

double AABB::getSurfaceArea() const
{
	auto diagonal = this->end - this->start;
	auto x = diagonal.x();
	auto y = diagonal.y();
	auto z = diagonal.z();
	return 2.0* (x * y + z * y + x * z);
}

AABB AABB::merge(const AABB& b) const
{
	auto& p1 = this->start;
	auto& p2 = this->end;
	auto& p3 = b.start;
	auto& p4 = b.end;

	const auto [startX, endX] = std::minmax({ p1.x(), p2.x(), p3.x(), p4.x() });
	const auto [startY, endY] = std::minmax({ p1.y(), p2.y(), p3.y(), p4.y() });
	const auto [startZ, endZ] = std::minmax({ p1.z(), p2.z(), p3.z(), p4.z() });
	const Point newStart(startX, startY, startZ);
	const Point newEnd(endX, endY, endZ);

	return AABB(newStart, newEnd);
}

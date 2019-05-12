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



double AABB::getSurfaceArea() const
{
	auto diagonal = this->end - this->start;
	auto x = diagonal.x();
	auto y = diagonal.y();
	auto z = diagonal.z();
	return 2.0 * (x * y + z * y + x * z);
}

AABB AABB::merge(const AABB& b) const
{
	const auto& p1 = this->start;
    const auto& p2 = this->end;
    const auto& p3 = b.start;
    const auto& p4 = b.end;

	const auto [startX, endX] = std::minmax({ p1.x(), p2.x(), p3.x(), p4.x() });
	const auto [startY, endY] = std::minmax({ p1.y(), p2.y(), p3.y(), p4.y() });
	const auto [startZ, endZ] = std::minmax({ p1.z(), p2.z(), p3.z(), p4.z() });
	const Point newStart(startX, startY, startZ);
	const Point newEnd(endX, endY, endZ);

	return AABB(newStart, newEnd);
}

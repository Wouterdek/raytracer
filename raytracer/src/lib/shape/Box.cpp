#include "Box.h"

Box::Box(Point start, Point end)
	: start(std::move(start)), end(std::move(end))
{ }

std::array<Point, 8> Box::getCorners() const
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

Point Box::getCentroid() const
{
	return this->start + ((this->end - this->start) / 2);
}

Box Box::getAABB() const
{
	return *this;
}

Box Box::getAABBOfTransformed(const Transformation& transform) const
{
	auto points = getCorners();
	for(int i = 0; i < points.size(); i++)
	{
		points[i] = transform.transform(points[i]);
	}
	auto[minXPoint, maxXPoint] = std::minmax_element(points.begin(), points.end(), [](const auto& p1, const auto& p2) { return p1.x() < p2.x(); });
	auto[minYPoint, maxYPoint] = std::minmax_element(points.begin(), points.end(), [](const auto& p1, const auto& p2) { return p1.y() < p2.y(); });
	auto[minZPoint, maxZPoint] = std::minmax_element(points.begin(), points.end(), [](const auto& p1, const auto& p2) { return p1.z() < p2.z(); });
	Point newStart(minXPoint->x(), minYPoint->y(), minZPoint->z());
	Point newEnd(maxXPoint->x(), maxYPoint->y(), maxZPoint->z());
	return Box(newStart, newEnd);
}

Vector3 Box::getNormal(const Ray& inverseRay, double t) const
{
	Point intersectionPoint = inverseRay.getOrigin() + (inverseRay.getDirection() * t);

	Vector3 center = this->start + ((this->end - this->start) * 0.5);
	Vector3 centerToIntersect = intersectionPoint - center;
	centerToIntersect.normalize();

	//TODO

	return centerToIntersect;
}

std::optional<RayHitInfo> Box::intersect(const Ray& ray) const
{
	Vector3 o = ray.getOrigin();
	Vector3 d = ray.getDirection();
	Vector3 s = this->start;
	Vector3 e = this->end;

	double tx_min, ty_min, tz_min;
	double tx_max, ty_max, tz_max;
	double a = 1.0 / d.x();
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

	double b = 1.0 / d.y();
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

	double c = 1.0 / d.z();
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

	double t0 = std::max(tx_min, ty_min);
	t0 = std::max(t0, tz_min);

	double t1 = std::min(tx_max, ty_max);
	t1 = std::min(t1, tz_max);

	if (t0 > t1)
	{
		return std::nullopt;
	}
	else
	{
		return RayHitInfo(ray, t0, this->getNormal(ray, t0));
	}
}

double Box::getSurfaceArea() const
{
	auto diagonal = this->end - this->start;
	auto x = diagonal.x();
	auto y = diagonal.y();
	auto z = diagonal.z();
	return 2.0*(x * y + z * y + x * z);
}

Box Box::merge(const Box & b) const
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

	return Box(newStart, newEnd);
}

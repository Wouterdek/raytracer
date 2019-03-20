#include "Box.h"

Box::Box()
	: AABB()
{
}

Box::Box(Point start, Point end)
	: AABB(std::move(start), std::move(end))
{
	assert(!hasNaN(start));
	assert(!hasNaN(end));
	assert(start.x() != end.x());
	assert(start.y() != end.y());
	assert(start.z() != end.z());
}

Point Box::getCentroid() const
{
	return this->start + ((this->end - this->start) / 2);
}

AABB Box::getAABB() const
{
	return *this;
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

	Vector2 texCoord; //TODO

	if (t0 > t1)
	{
		return std::nullopt;
	}
	else
	{
		return RayHitInfo(ray, t0, this->getNormal(ray, t0), texCoord);
	}
}

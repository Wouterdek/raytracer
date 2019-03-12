#include "Plane.h"
#include "Box.h"

Plane::Plane()
{
}

Point Plane::getCentroid() const
{
	return Point(0, 0, 0);
}

Box Plane::getAABB() const
{
	const auto inf = std::numeric_limits<float>::infinity();
	return Box(Point(-inf, 0, -inf), Point(inf, 0, inf));
}

std::optional<RayHitInfo> Plane::intersect(const Ray & ray) const
{
	const Vector3 normal { 0.0, 1.0, 0.0 };
	double q = ray.getDirection().dot(normal);
	if(q == 0)
	{
		return std::nullopt;
	}

	double t = ((ray.getOrigin() * -1).dot(normal))/q;

	if(t >= 0)
	{
		return RayHitInfo(ray, t, normal);
	}
	else
	{
		return std::nullopt;
	}
}

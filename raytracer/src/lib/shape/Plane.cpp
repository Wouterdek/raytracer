#include "Plane.h"
#include "Box.h"

Plane::Plane()
{
}

Point Plane::getCentroid() const
{
	return Point(0, 0, 0);
}

AABB Plane::getAABB() const
{
	const auto inf = std::numeric_limits<float>::infinity();
	auto maxFloat = std::numeric_limits<float>().max();
	return AABB(Point(-maxFloat, 0, -maxFloat), Point(maxFloat, 1E-6, maxFloat));
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

	Vector2 texCoord; //TODO
	Vector3 tangent; //TODO

	if(t >= 0)
	{
		return RayHitInfo(ray, t, normal, texCoord, tangent);
	}
	else
	{
		return std::nullopt;
	}
}

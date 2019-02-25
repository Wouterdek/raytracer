#include "Plane.h"

Plane::Plane()
{
}

std::optional<RayHitInfo> Plane::intersect(const Ray & ray, const Transformation & transform) const
{
	auto inverseRay = transform.transformInverse(ray);

	Vector3 normal { 0.0, 1.0, 0.0 };
	double q = inverseRay.getDirection().dot(normal);
	if(q == 0)
	{
		return std::nullopt;
	}

	double t = ((inverseRay.getOrigin() * -1).dot(normal))/q;

	if(t >= 0)
	{
		return RayHitInfo{ ray, t, transform.transform(normal)};
	}
	else
	{
		return std::nullopt;
	}
}

#include "RayHitInfo.h"

RayHitInfo::RayHitInfo(Ray ray, double t, Vector3 normal)
	: ray(std::move(ray)), t(t), normal(std::move(normal))
{
}

Point RayHitInfo::getHitpoint() const
{
	return ray.getOrigin() + (ray.getDirection() * t);
}

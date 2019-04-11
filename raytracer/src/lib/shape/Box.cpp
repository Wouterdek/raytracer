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
	float t = this->getIntersection(ray);

	Vector2 texCoord; //TODO

	if (t < 0)
	{
		return std::nullopt;
	}
	else
	{
		return RayHitInfo(ray, t, this->getNormal(ray, t), texCoord);
	}
}

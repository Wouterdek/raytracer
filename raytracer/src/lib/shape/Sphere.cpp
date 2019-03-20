#include "Sphere.h"
#include "Box.h"

Sphere::Sphere() = default;

Point Sphere::getCentroid() const
{
	return Point(0, 0, 0);
}

AABB Sphere::getAABB() const
{
	return Box(Point(-0.5, -0.5, -0.5), Point(0.5, 0.5, 0.5));
}

std::optional<RayHitInfo> Sphere::intersect(const Ray& ray) const
{
	auto o = Vector3(ray.getOrigin());

	auto direction = ray.getDirection();
	double a = direction.squaredNorm();
	double b = 2.0 * (direction.dot(o));
	double c = o.dot(o) - 1.0;

	double d = b * b - 4.0 * a * c;

	if (d < 0)
		return std::nullopt;
	double dr = sqrt(d);

	// numerically solve the equation a*t^2 + b * t + c = 0
	double q = -0.5 * (b < 0 ? (b - dr) : (b + dr));

	double t0 = q / a;
	double t1 = c / q;

	double smallestT = std::min(t0, t1);
	double largestT = std::min(t1, t0);

	double t;
	if(smallestT >= 0)
	{
		t = smallestT;
	}else if(largestT >= 0)
	{
		t = largestT;
	}else
	{
		return std::nullopt;
	}

	Point hitpoint = ray.getOrigin() + (t * ray.getDirection());
	Vector3 normal = hitpoint /* - origin */;

	Vector2 texCoord; //TODO

	return RayHitInfo(ray, t, normal, texCoord);
}

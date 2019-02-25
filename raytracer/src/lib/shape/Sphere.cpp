#include "Sphere.h"

std::optional<RayHitInfo> Sphere::intersect(const Ray& ray, const Transformation& transform) const
{
	auto transformed = transform.transformInverse(ray);
	
	auto o = Vector3(transformed.getOrigin());

	auto direction = transformed.getDirection();
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
	Point origin = Point{ 0.0, 0.0, 0.0 };
	Vector3 normal = hitpoint - transform.transform(origin);
	normal.normalize();

	return RayHitInfo(ray, t, normal);
}

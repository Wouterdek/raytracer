#include "Sphere.h"

Sphere::Sphere(Transformation transform)
	: transform(std::move(transform))
{ }

bool Sphere::intersect(const Ray& ray) const
{
	auto transformed = transform.transformInverse(ray);
	
	auto o = Vector3(transformed.getOrigin());

	auto direction = transformed.getDirection();
	double a = direction.squaredNorm();
	double b = 2.0 * (direction.dot(o));
	double c = o.dot(o) - 1.0;

	double d = b * b - 4.0 * a * c;

	if (d < 0)
		return false;
	double dr = sqrt(d);

	// numerically solve the equation a*t^2 + b * t + c = 0
	double q = -0.5 * (b < 0 ? (b - dr) : (b + dr));

	double t0 = q / a;
	double t1 = c / q;

	return t0 >= 0 || t1 >= 0;
}

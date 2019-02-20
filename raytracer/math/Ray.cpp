#include "ray.h"
#include <sstream>

Ray::Ray(const Point origin, const Vector3 dir)
	: origin(std::move(origin)), direction(std::move(dir))
{ }

std::ostream & operator<<(std::ostream & in, const Ray & ray)
{
	in << "[Ray] from "
		<< formatRow(ray.origin)
		<< " in direction "
		<< formatRow(ray.direction);
	return in;
}

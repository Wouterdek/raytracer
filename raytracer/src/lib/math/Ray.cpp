#include "Ray.h"
#include <sstream>

Ray::Ray(Point origin, Vector3 dir)
	: origin(std::move(origin))
{
    this->direction = std::move(dir);
}

std::ostream & operator<<(std::ostream & in, const Ray & ray)
{
	in << "[Ray] from "
		<< formatRow(ray.getOrigin())
		<< " in direction "
		<< formatRow(ray.getDirection());
	return in;
}

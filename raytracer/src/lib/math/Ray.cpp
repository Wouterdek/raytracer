#include "Ray.h"
#include <sstream>

Ray::Ray(Point origin, Vector3 dir)
	: origin(std::move(origin)), direction(std::move(dir))
{ }

const Point& Ray::getOrigin() const
{
	return this->origin;
}

const Vector3 Ray::getDirection() const
{
	return this->direction;
}

std::ostream & operator<<(std::ostream & in, const Ray & ray)
{
	in << "[Ray] from "
		<< formatRow(ray.getOrigin())
		<< " in direction "
		<< formatRow(ray.getDirection());
	return in;
}

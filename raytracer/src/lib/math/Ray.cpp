#include "Ray.h"
#include <sstream>

std::ostream & operator<<(std::ostream & in, const Ray & ray)
{
	in << "[Ray] from "
		<< formatRow(ray.getOrigin())
		<< " in direction "
		<< formatRow(ray.getDirection());
	return in;
}

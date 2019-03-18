#include "RayHitInfo.h"

Point RayHitInfo::getHitpoint() const
{
	return ray.getOrigin() + (ray.getDirection() * t);
}

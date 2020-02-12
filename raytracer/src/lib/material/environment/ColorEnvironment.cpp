#include "ColorEnvironment.h"

ColorEnvironment::ColorEnvironment() : color(RGB::BLACK)
{ }

RGB ColorEnvironment::getRadiance(const Scene &scene, const Vector3 &direction) const
{
    return color * intensity;
}

ColorEnvironment *ColorEnvironment::cloneImpl() const
{
    return new ColorEnvironment(*this);
}

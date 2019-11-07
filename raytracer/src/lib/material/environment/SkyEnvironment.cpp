#include "SkyEnvironment.h"

const RGB topColor {0.5, 0.7, 1.0};
const RGB secondaryColor {1.0, 1.0, 1.0};

RGB SkyEnvironment::getRadiance(const Scene &scene, const Vector3 &direction) const
{
    float t = 0.5f * (direction.y() + 1.0f);
    return (topColor.scale(t)) + (secondaryColor.scale((1.0f-t)));
}

SkyEnvironment* SkyEnvironment::cloneImpl() const
{
    return new SkyEnvironment();
}

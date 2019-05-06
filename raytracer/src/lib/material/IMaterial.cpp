#include "IMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

std::tuple<Vector3, RGB, float> IMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    return std::make_tuple(hit.ray.getDirection(), incomingEnergy, 0.0); //pass-through
}

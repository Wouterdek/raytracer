//
// Created by wouter on 15/04/19.
//

#include "PositionMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

PositionMaterial::PositionMaterial() = default;

RGB PositionMaterial::getColorFor(const SceneRayHitInfo &hit, const Scene &scene, int depth) const {
    auto hitpoint = hit.getHitpoint();
    return RGB(abs(hitpoint.x()), abs(hitpoint.y()), abs(hitpoint.z()));
}

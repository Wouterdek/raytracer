//
// Created by wouter on 15/04/19.
//

#include "PositionMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

PositionMaterial::PositionMaterial() = default;

RGB PositionMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    auto hitpoint = curNode.hit.getHitpoint();
    return RGB(abs(hitpoint.x()), abs(hitpoint.y()), abs(hitpoint.z()));
}


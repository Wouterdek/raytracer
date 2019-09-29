#include "TexCoordMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

TexCoordMaterial::TexCoordMaterial()
{ }

RGB TexCoordMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    const auto& hit = path[curI].hit;
    return RGB{ std::abs(hit.texCoord.x()), std::abs(hit.texCoord.y()), 0 };
}


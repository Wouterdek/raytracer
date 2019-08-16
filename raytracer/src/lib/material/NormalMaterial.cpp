#include "NormalMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

NormalMaterial::NormalMaterial()
{ }

RGB NormalMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    const auto& hit = curNode.hit;
    return RGB{ std::abs(hit.normal.x()), std::abs(hit.normal.y()), std::abs(hit.normal.z()) };
}


RGB NormalMaterial::getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const
{
	return RGB{ std::abs(hit.normal.x()), std::abs(hit.normal.y()), std::abs(hit.normal.z()) };
}

#include "TransparentMaterial.h"

void TransparentMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.specularity = 1.0;
    transport.isEmissive = false;
    transport.pathTerminationChance = 0.01;
    transport.type = TransportType::bounce; //technically transmission, but this is not a glass shader
    transport.transportDirection = transport.hit.ray.getDirection();
}

RGB TransparentMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode &curNode, const RGB &incomingEnergy) const
{
    return incomingEnergy;
}

std::tuple<Vector3, RGB, float> TransparentMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    return std::make_tuple(hit.ray.getDirection(), incomingEnergy, 0.0);
}

bool TransparentMaterial::hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const
{
    return false;
}

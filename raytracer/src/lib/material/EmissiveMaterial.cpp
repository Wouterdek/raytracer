#include "EmissiveMaterial.h"

EmissiveMaterial::EmissiveMaterial() = default;

void EmissiveMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.pathTerminationChance = 1.0;
    transport.isEmissive = true;
}

RGB EmissiveMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode &curNode, const RGB &incomingEnergy) const
{
    auto emissionColor = this->color;
    if(this->emissionMap != nullptr)
    {
        float x = abs(fmod(curNode.hit.texCoord.x(), 1.0f));
        float y = abs(fmod(curNode.hit.texCoord.y(), 1.0f));
        emissionColor = this->emissionMap->get(x * this->emissionMap->getWidth(), y * this->emissionMap->getHeight());
    }
    return emissionColor * this->intensity;
}

std::tuple<Vector3, RGB, float> EmissiveMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    return std::make_tuple(-hit.ray.getDirection(), RGB::BLACK, 0.0f);
}

bool EmissiveMaterial::hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const
{
    return false;
}

#include "AddMaterial.h"

//TODO: this material is buggy, as the pathtracing engine doesn't support calculating more than one branch at a time well.

void AddMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    first->sampleTransport(ctx);
    second->sampleTransport(ctx);
}

RGB AddMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode &curNode, const RGB &incomingEnergy) const
{
    RGB result1 = this->first->bsdf(scene, path, curI, curNode, incomingEnergy);
    RGB result2 = this->second->bsdf(scene, path, curI, curNode, incomingEnergy);
    return result1 + result2;
}

std::tuple<Vector3, RGB, float> AddMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    return this->first->interactPhoton(hit, incomingEnergy);
    //return this->second->interactPhoton(hit, incomingEnergy);
}

bool AddMaterial::hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const
{
    return first->hasVariance(path, curI, scene) || second->hasVariance(path, curI, scene);
}

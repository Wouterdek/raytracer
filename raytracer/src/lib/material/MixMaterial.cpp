#include "MixMaterial.h"
#include "math/FastRandom.h"

struct TransportMetaData
{
    bool choseFirst;
};

void MixMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    auto* meta = transport.metadata.alloc<TransportMetaData>();
    meta->choseFirst = Rand::unit() > mixFactor;
    transport.metadata.branch(meta->choseFirst);

    if(meta->choseFirst)
    {
        first->sampleTransport(ctx);
    }
    else
    {
        second->sampleTransport(ctx);
    }

    transport.metadata.up();
}

RGB MixMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    auto* meta = curNode.metadata.tryRead<TransportMetaData>();
    curNode.metadata.branch(meta->choseFirst);

    RGB result;
    if(meta->choseFirst)
    {
        result = this->first->bsdf(scene, path, curI, curNode, incomingEnergy);
    }
    else
    {
        result = this->second->bsdf(scene, path, curI, curNode, incomingEnergy);
    }

    curNode.metadata.up();
    return result;
}

std::tuple<Vector3, RGB, float> MixMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    if(Rand::unit() > mixFactor)
    {
        return this->first->interactPhoton(hit, incomingEnergy);
    }
    else
    {
        return this->second->interactPhoton(hit, incomingEnergy);
    }
}

bool MixMaterial::hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const
{
    bool noVariance = (this->mixFactor == 0.0f && !first->hasVariance(path, curI, scene))
            || (this->mixFactor == 1.0f && !second->hasVariance(path, curI, scene));
    return !noVariance;
}

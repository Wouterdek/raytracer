#pragma once

#include "IMaterial.h"
#include <memory>

class MixMaterial : public IMaterial {
public:
    void sampleTransport(TransportBuildContext &ctx) const override;
    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const SceneRayHitInfo &hit, const Scene &scene) const override;

    double mixFactor = 0.0; // 0 = 100% first, 1 = 100% second
    std::shared_ptr<IMaterial> first;
    std::shared_ptr<IMaterial> second;
};

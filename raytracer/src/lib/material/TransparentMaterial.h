#pragma once

#include "IMaterial.h"

class TransparentMaterial : public IMaterial
{
public:
    void sampleTransport(TransportBuildContext &ctx) const override;
    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const override;
};

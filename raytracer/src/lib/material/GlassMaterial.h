#pragma once

#include "IMaterial.h"
#include "film/RGB.h"

class GlassMaterial : public IMaterial
{
public:
    void sampleTransport(TransportBuildContext &ctx) const override;
    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const SceneRayHitInfo &hit, const Scene &scene) const override;

    double ior = 1.0;
    RGB color = RGB(1.0f);
    float attenuationStrength = 5.0;
};

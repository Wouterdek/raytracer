#pragma once

#include "IMaterial.h"
#include "Texture.h"
#include <memory>

class GlossyMaterial : public IMaterial
{
public:
    GlossyMaterial();

    void sampleTransport(TransportBuildContext &ctx) const override;
    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    virtual RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const SceneRayHitInfo &hit, const Scene &scene) const override;

    float roughness = 0.0f;
    std::shared_ptr<Texture> normalMap;
    RGB color = {1.0, 1.0, 1.0};
};
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

    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const override;

    float roughness = 0.0f;
    std::shared_ptr<TextureUInt8> normalMap;
    RGB color = {1.0, 1.0, 1.0};
};
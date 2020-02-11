#pragma once

#include "IMaterial.h"
#include "Texture.h"
#include <memory>

class EmissiveMaterial : public IMaterial
{
public:
    EmissiveMaterial();

    void sampleTransport(TransportBuildContext &ctx) const override;
    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const override;

    RGB color = RGB::BLACK;
    double intensity = 1.0;
    std::shared_ptr<TextureUInt8> emissionMap = nullptr;
};


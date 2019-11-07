#pragma once

#include "IMaterial.h"
#include "Texture.h"
#include <memory>

class DiffuseMaterial : public IMaterial
{
public:
	DiffuseMaterial();

    void sampleTransport(TransportBuildContext &ctx) const override;
    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const override;

    RGB diffuseColor = RGB::BLACK;
	double diffuseIntensity = 1.0;
	std::shared_ptr<Texture> albedoMap = nullptr;
	std::shared_ptr<Texture> normalMap = nullptr;
};

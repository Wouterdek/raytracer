#pragma once

#include "IMaterial.h"
#include "Texture.h"
#include <memory>

class DiffuseMaterial : public IMaterial
{
public:
	DiffuseMaterial();

	RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;

    RGB diffuseColor = RGB::BLACK;
	double diffuseIntensity = 1.0;
	std::shared_ptr<Texture> albedoMap;
	std::shared_ptr<Texture> normalMap;
};

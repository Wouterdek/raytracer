#pragma once

#include "IMaterial.h"
#include "Texture.h"
#include <memory>

class DiffuseMaterial : public IMaterial
{
public:
	DiffuseMaterial();
	RGB getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const override;

	RGB diffuseColor;
	RGB ambientColor;
	double ambientIntensity;
	double diffuseIntensity;
	std::shared_ptr<Texture> albedoMap;
	std::shared_ptr<Texture> normalMap;
};

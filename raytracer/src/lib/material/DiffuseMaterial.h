#pragma once

#include "IMaterial.h"
#include "Texture.h"

class DiffuseMaterial : public IMaterial
{
public:
	DiffuseMaterial();
	RGB getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const override;

	RGB diffuseColor;
	RGB ambientColor;
	double ambientIntensity;
	double diffuseIntensity;
	std::shared_ptr<Texture> albedo;
};

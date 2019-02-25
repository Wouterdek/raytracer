#pragma once

#include "IMaterial.h"

class DiffuseMaterial : public IMaterial
{
public:
	DiffuseMaterial();
	RGB getColorFor(const RayHitInfo& hit, const Scene& scene, int depth) override;

	RGB diffuseColor;
	RGB ambientColor;
	double ambientIntensity;
	double diffuseIntensity;
};

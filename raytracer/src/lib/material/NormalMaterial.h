#pragma once

#include "IMaterial.h"

class NormalMaterial : public IMaterial
{
public:
	NormalMaterial();
	RGB getColorFor(const RayHitInfo& hit, const Scene& scene, int depth) override;
};
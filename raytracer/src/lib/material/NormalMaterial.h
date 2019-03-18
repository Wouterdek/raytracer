#pragma once

#include "IMaterial.h"

class NormalMaterial : public IMaterial
{
public:
	NormalMaterial();
	RGB getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const override;
};
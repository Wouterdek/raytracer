#pragma once

#include "IMaterial.h"

class TexCoordMaterial : public IMaterial
{
public:
	TexCoordMaterial();
	RGB getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const override;
};
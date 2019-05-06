#pragma once

#include "IMaterial.h"

class NormalMaterial : public IMaterial
{
public:
	NormalMaterial();

	RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
};
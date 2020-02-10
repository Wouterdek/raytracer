#pragma once

#include "MixMaterial.h"

class FresnelMixMaterial : public MixMaterial {
public:
    float calcMixFactor(const SceneRayHitInfo &hit) const override;
    float IOR = 1.45f;
};
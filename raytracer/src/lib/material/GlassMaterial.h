#pragma once

#include "IMaterial.h"
#include "film/RGB.h"

class GlassMaterial : public IMaterial
{
public:
    RGB getColorFor(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;

    double ior = 1.0;
};

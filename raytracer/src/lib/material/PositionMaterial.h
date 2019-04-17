#pragma once

#include "IMaterial.h"

class PositionMaterial : public IMaterial{
public:
    PositionMaterial();
    virtual RGB getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const override;
};


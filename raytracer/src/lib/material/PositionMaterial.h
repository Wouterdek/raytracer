#pragma once

#include "IMaterial.h"

class PositionMaterial : public IMaterial{
public:
    PositionMaterial();

    virtual RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
};


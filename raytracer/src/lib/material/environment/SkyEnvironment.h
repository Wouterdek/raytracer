#pragma once

#include <film/RGB.h>
#include <math/Vector3.h>
#include <scene/renderable/Scene.h>
#include "IEnvironmentMaterial.h"

class SkyEnvironment : public IEnvironmentMaterial
{
    RGB getRadiance(const Scene& scene, const Vector3& direction) const override;

private:
    SkyEnvironment* cloneImpl() const override;
};

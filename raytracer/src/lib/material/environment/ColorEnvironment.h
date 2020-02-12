#pragma once

#include <film/RGB.h>
#include <scene/renderable/Scene.h>
#include "IEnvironmentMaterial.h"

class ColorEnvironment : public IEnvironmentMaterial
{
public:
    ColorEnvironment();
    RGB getRadiance(const Scene& scene, const Vector3& direction) const override;

    RGB color;
    double intensity = 1.0;
private:
    ColorEnvironment* cloneImpl() const override;
};

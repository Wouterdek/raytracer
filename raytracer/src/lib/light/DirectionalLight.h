#pragma once

#include "film/RGB.h"
#include "utility/ICloneable.h"
#include "math/Vector3.h"

class Transformation;

class DirectionalLight : public ICloneable<DirectionalLight>
{
public:
    DirectionalLight();

    RGB color;
    double intensity; // irradiance
    Vector3 direction;
    float angle; // radius size in hemisphere

    void applyTransform(const Transformation& transform);

private:
    DirectionalLight* cloneImpl() const override;
};

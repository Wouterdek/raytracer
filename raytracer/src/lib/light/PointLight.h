#pragma once

#include "film/RGB.h"
#include "utility/ICloneable.h"
#include "math/Vector3.h"

class Transformation;

class PointLight : public ICloneable<PointLight>
{
public:
	PointLight();

	RGB color;
	double intensity;
	Point pos;

	void applyTransform(const Transformation& transform);

private:
	PointLight* cloneImpl() override;
};

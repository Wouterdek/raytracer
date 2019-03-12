#pragma once

#include "film/RGB.h"
#include "utility/ICloneable.h"

class PointLamp : public ICloneable<PointLamp>
{
public:
	PointLamp();

	RGB color;
	double intensity;

private:
	PointLamp* cloneImpl() override;
};

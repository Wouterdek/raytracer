#pragma once

#include "../film/RGB.h"

class PointLamp
{
public:
	PointLamp();

	RGB color;
	double intensity;
};
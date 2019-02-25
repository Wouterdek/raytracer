#pragma once

#include "../math/Ray.h"
#include <Eigen/Dense>

using Sample = Eigen::Vector2d;

class ICamera
{
public:
	virtual ~ICamera() = default;
	virtual Ray generateRay(const Sample& sample) = 0;
};

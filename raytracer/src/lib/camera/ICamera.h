#pragma once

#include "math/Ray.h"
#include <Eigen/Dense>
#include "utility/ICloneable.h"

using Sample = Eigen::Vector2d;

class ICamera : public ICloneable<ICamera>
{
public:
	virtual ~ICamera() = default;
	virtual Ray generateRay(const Sample& sample) const = 0;
};

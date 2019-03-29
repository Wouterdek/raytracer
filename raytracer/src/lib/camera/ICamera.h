#pragma once

#include "math/Ray.h"
#include "math/Transformation.h"
#include <Eigen/Dense>
#include "utility/ICloneable.h"

using Sample = Eigen::Vector2d;

class ICamera : public ICloneable<ICamera>
{
public:
	virtual ~ICamera() = default;
	virtual Ray generateRay(const Sample& sample, int xResolution, int yResolution) const = 0;

	virtual void setTransform(const Transformation& transform) = 0;
};

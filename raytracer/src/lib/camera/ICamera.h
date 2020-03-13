#pragma once

#include "math/Ray.h"
#include "math/Vector2.h"
#include "math/Transformation.h"
#include <Eigen/Dense>
#include "utility/ICloneable.h"

class ICamera : public ICloneable<ICamera>
{
public:
	virtual ~ICamera() = default;
	virtual Ray generateRay(const Vector2& pixel, int xResolution, int yResolution, int aaLevel, int sampleI) const = 0;

	virtual void setTransform(const Transformation& transform) = 0;

	double focalDistance = 0;
	double aperture = 0;
    bool isMainCamera = false;
};

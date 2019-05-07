#pragma once

#include "ICamera.h"
#include "math/Vector3.h"
#include "math/Ray.h"
#include "math/OrthonormalBasis.h"

class Transformation;

class PerspectiveCamera : public ICamera
{
public:
	explicit PerspectiveCamera(double fov);
	Ray generateRay(const Vector2& sample, int xResolution, int yResolution) const override;

	void setTransform(const Transformation& transform) override;
	void pointAt(Point worldSpaceTarget, Vector3 up);

private:
	PerspectiveCamera* cloneImpl() const override;

	Point origin;
	OrthonormalBasis basis;
	double width;
};

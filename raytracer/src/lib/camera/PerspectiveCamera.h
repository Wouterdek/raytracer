#pragma once

#include "ICamera.h"
#include "math/Vector3.h"
#include "math/Ray.h"
#include "math/OrthonormalBasis.h"

class Transformation;

class PerspectiveCamera : public ICamera
{
public:
	PerspectiveCamera(double fov);
	Ray generateRay(const Sample& sample, int xResolution, int yResolution) const override;

	void setTransform(const Transformation& transform) override;
	void pointAt(Point worldSpaceTarget, Vector3 up);

private:
	PerspectiveCamera* cloneImpl() override;

	Point origin;
	OrthonormalBasis basis;
	double width;
};

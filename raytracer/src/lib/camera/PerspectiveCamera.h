#pragma once

#include "ICamera.h"
#include "math/Vector3.h"
#include "math/Ray.h"
#include "math/OrthonormalBasis.h"

class PerspectiveCamera : public ICamera
{
public:
	PerspectiveCamera(int xResolution, int yResolution, Point origin, Vector3 lookat, Vector3 up, double fov);
	static PerspectiveCamera with_destination_point(int xResolution, int yResolution, Point origin, const Point& destination, Vector3 up, double fov);
	Ray generateRay(const Sample& sample) const override;

private:
	PerspectiveCamera* cloneImpl() override;

	Point origin;
	OrthonormalBasis basis;
	double width;
	double height;
	double invxResolution;
	double invyResolution;
};

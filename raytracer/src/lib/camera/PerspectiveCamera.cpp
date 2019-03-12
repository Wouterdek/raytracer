#include "PerspectiveCamera.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"

PerspectiveCamera::PerspectiveCamera(int xResolution, int yResolution, Point origin, Vector3 lookat, Vector3 up, double fov)
  : origin(std::move(origin)), 
	basis(OrthonormalBasis(-lookat, up)),
	invxResolution(1.0/xResolution),
	invyResolution(1.0/yResolution)
{
	width = 2.0 * tan(0.5 * (fov * PI / 180.0));
	height = yResolution * width * invxResolution;
}

PerspectiveCamera PerspectiveCamera::with_destination_point(int xResolution, int yResolution, Point origin, const Point & destination, Vector3 up, double fov)
{
	return PerspectiveCamera(xResolution, yResolution, origin, destination - origin, up, fov);
}

Ray PerspectiveCamera::generateRay(const Sample& sample) const
{
	auto u = this->width * (sample.x() * invxResolution - 0.5);
	auto v = -this->height * (sample.y() * invyResolution - 0.5);

	auto direction = (basis.getU() * u) + (basis.getV() * v) - basis.getW();

	return Ray(origin, direction);
}

PerspectiveCamera* PerspectiveCamera::cloneImpl()
{
	return new PerspectiveCamera(*this);
}

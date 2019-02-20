#include "PerspectiveCamera.h"
#include "../math/OrthonormalBasis.h"

const double PI = 3.14159265358979323;

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

PerspectiveCamera::~PerspectiveCamera()
{
}

Ray PerspectiveCamera::generateRay(const Sample& sample)
{
	auto u = this->width * (sample.x() * invxResolution - 0.5);
	auto v = this->height * (sample.y() * invyResolution - 0.5);

	auto direction = (basis.getU() * u) + (basis.getV() * v) - basis.getW();

	return Ray(origin, direction);
}

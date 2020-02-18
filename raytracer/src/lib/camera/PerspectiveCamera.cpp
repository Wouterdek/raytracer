#include "PerspectiveCamera.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"
#include "math/Transformation.h"
#include "math/Sampler.h"

PerspectiveCamera::PerspectiveCamera(double fov)
  : basis(Vector3(0, 0, 1), Vector3(0, 1, 0))
{
	width = 2.0 * tan(0.5 * (fov * PI / 180.0)); // * 1 ( = distance sensor <-> lens)
}

void PerspectiveCamera::setTransform(const Transformation& transform)
{
	this->origin = transform.transform(Point(0, 0, 0));
	auto lookat = transform.transform(Vector3(0, 0, -1));
	auto up = transform.transform(Vector3(0, 1, 0));
	this->basis = OrthonormalBasis(-lookat, up);
}

void PerspectiveCamera::pointAt(Point worldSpaceTarget, Vector3 up)
{
	this->basis = OrthonormalBasis(-(worldSpaceTarget - origin), up);
}

Ray PerspectiveCamera::generateRay(const Vector2& sample, int xResolution, int yResolution) const
{
	auto height = yResolution * width / xResolution;
	auto u = this->width * (sample.x() / xResolution - 0.5);
	auto v = -height * (sample.y() / yResolution - 0.5);

	Vector3 direction = (basis.getU() * u) + (basis.getV() * v) - basis.getW(); // * 1 ( = distance sensor <-> lens)
    direction.normalize();

    Ray perfectRay(origin, direction);

    if(this->aperture == 0){
        return perfectRay;
    }

    Point focalPlanePoint = perfectRay.getOrigin() + (perfectRay.getDirection() * this->focalDistance);

    Point lensPoint = origin;
    auto offset = sampleUniformCircle(this->aperture);
    lensPoint += this->basis.getU() * offset.x();
    lensPoint += this->basis.getV() * offset.y();

    return Ray(lensPoint, (focalPlanePoint - lensPoint).normalized());
}

PerspectiveCamera* PerspectiveCamera::cloneImpl() const
{
	return new PerspectiveCamera(*this);
}

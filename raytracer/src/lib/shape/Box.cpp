#include "Box.h"

Box::Box(Vector3 end)
	: end(end)
{
}

std::optional<RayHitInfo> Box::intersect(const Ray & ray, const Transformation & transform) const
{
	auto inverseRay = transform.transformInverse(ray);

	Vector3 o = inverseRay.getOrigin();
	Vector3 d = inverseRay.getDirection();

	double tx_min, ty_min, tz_min;
	double tx_max, ty_max, tz_max;
	double a = 1.0 / d.x();
	if(a >= 0)
	{
		tx_min = -o.x() * a;
		tx_max = (end.x() - o.x()) * a;
	}
	else
	{
		tx_min = (end.x() - o.x()) * a;
		tx_max = -o.x() * a;
	}
	
	double b = 1.0 / d.y();
	if (b >= 0)
	{
		ty_min = -o.y() * b;
		ty_max = (end.y() - o.y()) * b;
	}
	else
	{
		ty_min = (end.y() - o.y()) * b;
		ty_max = -o.y() * b;
	}

	double c = 1.0 / d.z();
	if (c >= 0)
	{
		ty_min = -o.z() * c;
		ty_max = (end.z() - o.z()) * c;
	}
	else
	{
		ty_min = (end.z() - o.z()) * c;
		ty_max = -o.z() * b;
	}

	double t0 = std::max(tx_min, ty_min);
	t0 = std::max(t0, tz_min);

	double t1 = std::min(tx_max, ty_max);
	t1 = std::min(t1, tz_max);

	if(t0 > t1)
	{
		return std::nullopt;
	}else
	{
		return RayHitInfo(ray, t0, Vector3{ 1.0, 0.0, 0.0 }); //TODO
	}
}

#include "NormalMaterial.h"

NormalMaterial::NormalMaterial()
{ }

RGB NormalMaterial::getColorFor(const RayHitInfo& hit, const Scene& scene, int depth) const
{
	return RGB{ std::abs(hit.normal.x()), std::abs(hit.normal.y()), std::abs(hit.normal.z()) };
}

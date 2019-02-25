#include "NormalMaterial.h"

NormalMaterial::NormalMaterial()
{ }

RGB NormalMaterial::getColorFor(const RayHitInfo& hit, const Scene& scene, int depth)
{
	return RGB{ std::abs(hit.normal.x()), std::abs(hit.normal.y()), std::abs(hit.normal.z()) };
}

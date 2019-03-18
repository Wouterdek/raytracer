#include "NormalMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

NormalMaterial::NormalMaterial()
{ }

RGB NormalMaterial::getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const
{
	return RGB{ std::abs(hit.normal.x()), std::abs(hit.normal.y()), std::abs(hit.normal.z()) };
}

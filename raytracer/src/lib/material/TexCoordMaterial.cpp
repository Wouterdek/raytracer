#include "TexCoordMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

TexCoordMaterial::TexCoordMaterial()
{ }

RGB TexCoordMaterial::getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const
{
	return RGB{ std::abs(hit.texCoord.x()), std::abs(hit.texCoord.y()), 0 };
}

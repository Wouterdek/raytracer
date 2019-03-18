#include "TexCoordMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"

TexCoordMaterial::TexCoordMaterial()
{ }

RGB TexCoordMaterial::getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const
{
	return RGB{ hit.texCoord.x(), hit.texCoord.y(), 0 };
}

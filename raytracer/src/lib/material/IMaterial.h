#pragma once
#include "film/RGB.h"
#include "math/RayHitInfo.h"

class SceneRayHitInfo;
class Scene;

class IMaterial
{
public:
	virtual ~IMaterial() = default;

	virtual RGB getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const = 0;
};

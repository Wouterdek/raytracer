#pragma once
#include "../film/RGB.h"
#include "../math/RayHitInfo.h"

class Scene;

class IMaterial
{
public:
	virtual ~IMaterial() = default;

	virtual RGB getColorFor(const RayHitInfo& hit, const Scene& scene, int depth) = 0;
};

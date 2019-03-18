#include "SceneRayHitInfo.h"

SceneRayHitInfo::SceneRayHitInfo(RayHitInfo geometryInfo, const SceneNode<Model>& node)
	: RayHitInfo(std::move(geometryInfo)), modelNode(std::reference_wrapper<const SceneNode<Model>>(node))
{ }

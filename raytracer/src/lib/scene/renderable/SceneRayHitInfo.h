#pragma once

#include "math/RayHitInfo.h"
#include "SceneNode.h"
#include "model/Model.h"

class SceneRayHitInfo
{
public:
	SceneRayHitInfo(RayHitInfo geometryInfo, const SceneNode<Model>& node);

	const RayHitInfo& getGeometryInfo() const
	{
		return this->geometryInfo;
	}

	const SceneNode<Model>& getModelNode() const
	{
		return this->modelNode;
	}
private:
	RayHitInfo geometryInfo;
	std::reference_wrapper<const SceneNode<Model>> modelNode;
};

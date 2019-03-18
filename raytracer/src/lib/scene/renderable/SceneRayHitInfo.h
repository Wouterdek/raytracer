#pragma once

#include "math/RayHitInfo.h"
#include "SceneNode.h"
#include "model/Model.h"

class SceneRayHitInfo : public RayHitInfo
{
public:
	SceneRayHitInfo(RayHitInfo geometryInfo, const SceneNode<Model>& node);

	const SceneNode<Model>& getModelNode() const
	{
		return this->modelNode;
	}

private:
	std::reference_wrapper<const SceneNode<Model>> modelNode;
};

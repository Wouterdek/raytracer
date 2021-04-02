#pragma once

#include "math/RayHitInfo.h"

template<typename T>
class SceneNode;
class Model;

class SceneRayHitInfo : public RayHitInfo
{
public:
	SceneRayHitInfo(RayHitInfo geometryInfo, const SceneNode<Model>& node) : RayHitInfo(geometryInfo), modelNode(node) {};

	const SceneNode<Model>& getModelNode() const
	{
		return this->modelNode;
	}

private:
	std::reference_wrapper<const SceneNode<Model>> modelNode;
};

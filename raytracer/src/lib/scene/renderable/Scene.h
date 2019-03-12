#pragma once

#include <optional>
#include "math/RayHitInfo.h"
#include "model/Model.h"
#include "lamp/PointLamp.h"
#include "camera/ICamera.h"
#include "shape/list/IShapeList.h"
#include "shape/bvh/BVH.h"
#include "SceneRayHitInfo.h"
#include "SceneNode.h"

class Scene
{
public:
	using SceneBVH = BVH<IShapeList<SceneRayHitInfo>, SceneRayHitInfo, 2>;

	Scene(
		std::vector<SceneNode<PointLamp>>&& lights,
		std::vector<SceneNode<ICamera>>&& cameras,
		SceneBVH&& sceneBVH
	);

	const std::vector<SceneNode<PointLamp>>& getLights() const;
	const std::vector<SceneNode<ICamera>>& getCameras() const;

	std::optional<SceneRayHitInfo> traceRay(const Ray& ray) const;

private:
	std::vector<SceneNode<PointLamp>> lights;
	std::vector<SceneNode<ICamera>> cameras;
	SceneBVH sceneBVH;
};

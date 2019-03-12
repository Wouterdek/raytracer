#include "DynamicScene.h"
#include "shape/list/InstancedModelList.h"
#include "shape/bvh/BVHBuilder.h"

DynamicScene::DynamicScene()
{
	this->root = std::make_unique<DynamicSceneNode>();
}

Scene DynamicScene::build() const
{
	// Flatten scene
	std::vector<SceneNode<PointLamp>> lights{};
	std::vector<SceneNode<ICamera>> cameras{};
	std::vector<SceneNode<Model>> models{};

	this->walkDepthFirst<Transformation>([&lights, &cameras, &models](const DynamicSceneNode& node, const Transformation& parentTransform)
	{
		auto transform = parentTransform.append(node.transform);

		if (node.lamp != nullptr)
		{
			lights.emplace_back(transform, node.lamp->clone());
		}
		if(node.camera != nullptr)
		{
			cameras.emplace_back(transform, node.camera->clone());
		}
		if (node.model != nullptr)
		{
			models.emplace_back(transform, node.model->clone());
		}

		return std::make_pair(transform, true);
	}, Transformation::IDENTITY);

	// Calculate scene BVH
	InstancedModelList modelList(std::move(models));
	modelList.buildShapeBVHCache();

	auto sceneBVH = BVHBuilder<SceneRayHitInfo>::buildBVH(modelList);
	return Scene(std::move(lights), std::move(cameras), std::move(sceneBVH));
}

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
	std::vector<std::unique_ptr<PointLight>> pointLights{};
	std::vector<std::unique_ptr<AreaLight>> areaLights{};
	std::vector<SceneNode<ICamera>> cameras{};
	std::vector<SceneNode<Model>> models{};

	this->walkDepthFirst<Transformation>([&pointLights, &areaLights, &cameras, &models](const DynamicSceneNode& node, const Transformation& parentTransform)
	{
		auto transform = parentTransform.append(node.transform);

		if (node.pointLight != nullptr)
		{
			auto light = node.pointLight->clone();
			light->applyTransform(transform);
			pointLights.push_back(std::move(light));
		}
		if (node.areaLight != nullptr)
		{
			auto light = node.areaLight->clone();
			light->applyTransform(transform);
			areaLights.push_back(std::move(light));
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
	return Scene(std::move(pointLights), std::move(areaLights), std::move(cameras), std::move(sceneBVH));
}

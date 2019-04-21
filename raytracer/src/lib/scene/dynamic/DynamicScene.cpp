#include "DynamicScene.h"
#include "shape/list/InstancedModelList.h"
#include "shape/bvh/BVHBuilder.h"
#include "shape/TriangleMesh.h"
#include "material/CompositeMaterial.h"
#include <iostream>

DynamicScene::DynamicScene()
{
	this->root = std::make_unique<DynamicSceneNode>();
}

DynamicScene DynamicScene::soupifyScene() const {
    DynamicScene result {};
    result.root = std::make_unique<DynamicSceneNode>();

    auto mergedMesh = std::make_shared<TriangleMesh>(true);
    auto mergedMaterial = std::make_shared<CompositeMaterial>();
    using Accumulator = std::pair<DynamicSceneNode*, Transformation>;
    this->walkDepthFirst<Accumulator>([&mergedMesh, &mergedMaterial](const DynamicSceneNode& node, const Accumulator& acc){
        auto& [parentResult, parentTransform] = acc;

        auto resultNode = std::make_unique<DynamicSceneNode>();
        auto resultNodePtr = &(*resultNode);
        resultNode->transform = node.transform;

        resultNode->camera = node.camera == nullptr ? nullptr : node.camera->clone();
        resultNode->areaLight = node.areaLight == nullptr ? nullptr : node.areaLight->clone();
        resultNode->pointLight = node.pointLight == nullptr ? nullptr : node.pointLight->clone();

        auto transform = parentTransform.append(node.transform);
        if(node.model != nullptr){
            TriangleMesh* curMesh = dynamic_cast<TriangleMesh*>(node.model->getShapePtr().get());
            if(curMesh != nullptr){
                mergedMaterial->addMaterial(mergedMesh->count(), curMesh->count(), node.model->getMaterialPtr());

                std::shared_ptr<TriangleMeshData> clonedMeshData = curMesh->getData().clone();
                TriangleMesh meshClone(clonedMeshData, curMesh->getBeginIndex(), curMesh->getEndIndex());
                meshClone.applyTransform(transform);
                mergedMesh->appendMesh(meshClone);
            }
        }
        parentResult->children.push_back(std::move(resultNode));

        return std::make_pair(std::make_pair(resultNodePtr, transform), true);
    }, Accumulator(&(*result.root), Transformation::IDENTITY));

    auto meshNode = std::make_unique<DynamicSceneNode>();
    meshNode->model = std::make_unique<Model>(mergedMesh, mergedMaterial);
    result.root->children.push_back(std::move(meshNode));
    return result;
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
			auto& camera = cameras.emplace_back(transform, node.camera->clone());
			camera.getData().setTransform(transform);
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
	std::cout << "Top level BVH: " << sceneBVH.getSize() << std::endl;
	return Scene(std::move(pointLights), std::move(areaLights), std::move(cameras), std::move(sceneBVH));
}

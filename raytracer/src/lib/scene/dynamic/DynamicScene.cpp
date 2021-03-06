#include "DynamicScene.h"
#include "shape/list/InstancedModelList.h"
#include "shape/bvh/BVHBuilder.h"
#include "shape/TriangleMesh.h"
#include "material/CompositeMaterial.h"
#include <iostream>

#ifndef NO_TBB
#include <tbb/tbb.h>
#endif

DynamicScene::DynamicScene()
{
	this->root = std::make_unique<DynamicSceneNode>();
}

DynamicScene DynamicScene::soupifyScene(Statistics::Collector* stats) const
{
    DynamicScene result {};
    if(this->environmentMaterial != nullptr)
    {
        result.environmentMaterial = this->environmentMaterial->clone();
    }
    result.root = std::make_unique<DynamicSceneNode>();

    // Gather instantiations of meshes (+materials), copy over other nodes (lights, cameras, ...)
    std::vector<Transformation> instTransforms;
    std::vector<TriangleMesh*> instMeshes;
    std::vector<std::shared_ptr<IMaterial>> instMaterials;
    using Accumulator = std::pair<DynamicSceneNode*, Transformation>;
    this->walkDepthFirst<Accumulator>([&instTransforms, &instMeshes, &instMaterials](const DynamicSceneNode& node, const Accumulator& acc){
        auto& [parentResult, parentTransform] = acc;

        auto resultNode = std::make_unique<DynamicSceneNode>();
        auto resultNodePtr = &(*resultNode);
        resultNode->transform = node.transform;

        resultNode->camera = node.camera == nullptr ? nullptr : node.camera->clone();
        resultNode->areaLight = node.areaLight == nullptr ? nullptr : node.areaLight->clone();
        resultNode->pointLight = node.pointLight == nullptr ? nullptr : node.pointLight->clone();
        resultNode->directionalLight = node.directionalLight == nullptr ? nullptr : node.directionalLight->clone();

        auto transform = parentTransform.append(node.transform);
        if(node.model != nullptr)
        {
            TriangleMesh* curMesh = dynamic_cast<TriangleMesh*>(node.model->getShapePtr().get());
            if(curMesh == nullptr)
            {
                resultNode->model = node.model->clone();
            }
            else
            {
                instTransforms.emplace_back(transform);
                instMeshes.emplace_back(curMesh);
                instMaterials.emplace_back(node.model->getMaterialPtr());
            }
        }
        parentResult->children.push_back(std::move(resultNode));

        return std::make_pair(std::make_pair(resultNodePtr, transform), true);
    }, Accumulator(&(*result.root), Transformation::IDENTITY));

    // Merge meshes, materials
    auto instanceCount = instTransforms.size();
    auto mergedMesh = std::make_shared<TriangleMesh>(true);
    auto subMeshes = mergedMesh->appendMeshes(instMeshes);

    auto mergedMaterial = std::make_shared<CompositeMaterial>();
    for(ulong i = 0; i < instanceCount; ++i)
    {
        mergedMaterial->addMaterial(subMeshes[i].getBeginIndex(), subMeshes[i].count(), instMaterials[i]);
    }

    // Apply transformations
#ifdef NO_TBB
    for(ulong i = 0; i < instanceCount; ++i)
    {
        TriangleMesh& mesh = subMeshes[i];
        Transformation& transform = instTransforms[i];
        mesh.applyTransform(transform);
    }
#else
    tbb::parallel_for(0ul, instanceCount, [&instTransforms, &subMeshes](auto i){
        TriangleMesh& mesh = subMeshes[i];
        Transformation& transform = instTransforms[i];
        mesh.applyTransform(transform);
    });
#endif

    LOGSTAT(stats, "TriangleCount", mergedMesh->count());
    LOGSTAT(stats, "ModelsMerged", instanceCount);

    auto meshNode = std::make_unique<DynamicSceneNode>();
    meshNode->model = std::make_unique<Model>(mergedMesh, mergedMaterial);
    result.root->children.push_back(std::move(meshNode));
    return result;
}

Scene DynamicScene::build(Statistics::Collector* stats) const
{
	// Flatten scene
	std::vector<std::unique_ptr<PointLight>> pointLights{};
	std::vector<std::unique_ptr<AreaLight>> areaLights{};
    std::vector<std::unique_ptr<DirectionalLight>> directionalLights{};
	std::vector<SceneNode<ICamera>> cameras{};
	std::vector<SceneNode<Model>> models{};

	this->walkDepthFirst<Transformation>([&pointLights, &areaLights, &directionalLights, &cameras, &models](const DynamicSceneNode& node, const Transformation& parentTransform)
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
        if (node.directionalLight != nullptr)
        {
            auto light = node.directionalLight->clone();
            light->applyTransform(transform);
            directionalLights.push_back(std::move(light));
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

    LOGSTAT(stats, "PointLightCount", pointLights.size());
    LOGSTAT(stats, "AreaLightCount", areaLights.size());
    LOGSTAT(stats, "DirectionalLightCount", directionalLights.size());
    LOGSTAT(stats, "CameraCount", cameras.size());
    LOGSTAT(stats, "ModelCount", models.size());

	// Calculate scene BVH
	InstancedModelList modelList(std::move(models));
	modelList.buildShapeBVHCache(stats);

	auto sceneBVH = BVHBuilder<SceneRayHitInfo>::buildBVH(modelList, stats);
    LOGSTAT(stats, "TopLevelBVHNodeCount", sceneBVH.getSize());
	Scene scene(std::move(pointLights), std::move(areaLights), std::move(directionalLights), std::move(cameras), std::move(sceneBVH));
	if(this->environmentMaterial != nullptr)
    {
        scene.setEnvironmentMaterial(this->environmentMaterial->clone());
    }
	return scene;
}

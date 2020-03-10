#pragma once

#include <optional>
#include "math/RayHitInfo.h"
#include "model/Model.h"
#include "camera/ICamera.h"
#include "shape/list/IShapeList.h"
#include "shape/bvh/BVH.h"
#include "SceneRayHitInfo.h"
#include "SceneNode.h"
#include "light/PointLight.h"
#include "light/AreaLight.h"
#include "light/DirectionalLight.h"
#include "material/environment/IEnvironmentMaterial.h"
#include "photonmapping/PhotonMap.h"

class Scene
{
public:
	using SceneBVH = BVH<IShapeList<SceneRayHitInfo>, SceneRayHitInfo, 2>;

	Scene(
		std::vector<std::unique_ptr<PointLight>>&& pointLights,
		std::vector<std::unique_ptr<AreaLight>>&& areaLights,
        std::vector<std::unique_ptr<DirectionalLight>>&& directionalLights,
		std::vector<SceneNode<ICamera>>&& cameras,
		SceneBVH&& sceneBVH
	);

	const std::vector<std::unique_ptr<PointLight>>& getPointLights() const
	{
		return this->pointLights;
	}

	const std::vector<std::unique_ptr<AreaLight>>& getAreaLights() const
	{
		return this->areaLights;
	}

    const std::vector<std::unique_ptr<DirectionalLight>>& getDirectionalLights() const
    {
        return this->directionalLights;
    }

	const std::vector<SceneNode<ICamera>>& getCameras() const
	{
		return this->cameras;
	}

    bool hasEnvironmentMaterial() const
    {
        return environmentMaterial != nullptr;
    }

    const IEnvironmentMaterial& getEnvironmentMaterial() const
    {
        return *environmentMaterial;
    }

    void setEnvironmentMaterial(std::unique_ptr<IEnvironmentMaterial> material)
    {
        environmentMaterial = std::move(material);
    }

    const std::optional<PhotonMap>& getPhotonMap() const
    {
	    return photonMap;
    }

    void setPhotonMap(PhotonMap map)
    {
	    photonMap = std::move(map);
    }

    PhotonMapMode getPhotonMapMode() const
    {
        return photonMappingMode;
    }

    void setPhotonMapMode(PhotonMapMode mode)
    {
        photonMappingMode = mode;
    }

    void setPhotonMapDepth(int depth)
    {
        photonMapDepth = depth;
    }

    int getPhotonMapDepth() const
    {
	    return photonMapDepth;
    }

    // Get first hit
	std::optional<SceneRayHitInfo> traceRay(const Ray& ray) const;
    HitBundle<SceneRayHitInfo> traceRays(RayBundle& ray) const;

	// Get any hit between origin and maxT
    std::optional<SceneRayHitInfo> testVisibility(const Ray& ray, float maxT) const;

private:
	std::vector<std::unique_ptr<PointLight>> pointLights;
	std::vector<std::unique_ptr<AreaLight>> areaLights;
    std::vector<std::unique_ptr<DirectionalLight>> directionalLights;
	std::vector<SceneNode<ICamera>> cameras;
	std::unique_ptr<IEnvironmentMaterial> environmentMaterial = nullptr;
	std::optional<PhotonMap> photonMap;
    PhotonMapMode photonMappingMode = PhotonMapMode::none;
    int photonMapDepth = 0;
	SceneBVH sceneBVH;
};

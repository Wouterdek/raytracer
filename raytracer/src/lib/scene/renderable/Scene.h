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
#include "photonmapping/PhotonMap.h"

class Scene
{
public:
	using SceneBVH = BVH<IShapeList<SceneRayHitInfo>, SceneRayHitInfo, 2>;

	Scene(
		std::vector<std::unique_ptr<PointLight>>&& pointLights,
		std::vector<std::unique_ptr<AreaLight>>&& areaLights,
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

	const std::vector<SceneNode<ICamera>>& getCameras() const
	{
		return this->cameras;
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

    // Get first hit
	std::optional<SceneRayHitInfo> traceRay(const Ray& ray) const;

	// Get any hit between origin and maxT
    std::optional<SceneRayHitInfo> testVisibility(const Ray& ray, float maxT) const;

private:
	std::vector<std::unique_ptr<PointLight>> pointLights;
	std::vector<std::unique_ptr<AreaLight>> areaLights;
	std::vector<SceneNode<ICamera>> cameras;
	std::optional<PhotonMap> photonMap;
    PhotonMapMode photonMappingMode = PhotonMapMode::none;
	SceneBVH sceneBVH;
};

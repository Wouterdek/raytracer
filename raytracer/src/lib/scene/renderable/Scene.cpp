#include "Scene.h"

Scene::Scene(
	std::vector<std::unique_ptr<PointLight>>&& pointLights,
	std::vector<std::unique_ptr<AreaLight>>&& areaLights,
	std::vector<SceneNode<ICamera>>&& cameras, 
	SceneBVH&& sceneBVH
)
	: pointLights(std::move(pointLights)), areaLights(std::move(areaLights)), cameras(std::move(cameras)), sceneBVH(std::move(sceneBVH))
{ }

std::optional<SceneRayHitInfo> Scene::traceRay(const Ray& ray) const
{
	return this->sceneBVH.traceRay(ray);
}

std::optional<SceneRayHitInfo> Scene::testVisibility(const Ray &ray, float maxT) const
{
    return this->sceneBVH.testVisibility(ray, maxT);
}

#include "Scene.h"

Scene::Scene(
	std::vector<SceneNode<PointLamp>>&& lights, std::vector<SceneNode<ICamera>>&& cameras, SceneBVH&& sceneBVH)
	: lights(std::move(lights)), cameras(std::move(cameras)), sceneBVH(std::move(sceneBVH))
{ }

const std::vector<SceneNode<PointLamp>>& Scene::getLights() const
{
	return this->lights;
}

const std::vector<SceneNode<ICamera>>& Scene::getCameras() const
{
	return this->cameras;
}

std::optional<SceneRayHitInfo> Scene::traceRay(const Ray& ray) const
{
	return this->sceneBVH.traceRay(ray);
}

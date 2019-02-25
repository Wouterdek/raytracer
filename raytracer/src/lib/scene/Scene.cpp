#include "Scene.h"

Scene::Scene()
{
	this->root = std::make_unique<SceneNode>();
}

std::optional<std::pair<RayHitInfo, std::reference_wrapper<Model>>> Scene::traceRay(const Ray & ray) const
{
	std::optional<RayHitInfo> hit;
	std::optional<std::reference_wrapper<Model>> hitModel;
	this->walkDepthFirst<Transformation>([&ray, &hit, &hitModel](const SceneNode& node, const Transformation& t)
	{
		Transformation cur = t.append(node.transform);

		if (node.model != nullptr) {
			std::optional<RayHitInfo> curHit = node.model->shape->intersect(ray, cur);
			if (curHit.has_value() && (!hit.has_value() || curHit->t < hit->t))
			{
				hit = curHit;
				hitModel = *node.model;
			}
		}

		return std::make_pair(cur, true);
	}, Transformation::IDENTITY);

	if(hit.has_value())
	{
		return std::make_pair(*hit, *hitModel);
	}else
	{
		return std::nullopt;
	}
}


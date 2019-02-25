#pragma once
#include "SceneNode.h"
#include <memory>
#include <functional>
#include <optional>

class Scene
{
public:
	Scene();

	template<typename TState>
	std::optional<TState> walkDepthFirst(std::function<std::pair<TState, bool>(const SceneNode&, const TState&)> visitor, const TState& init) const
	{
		return this->root->walkDepthFirst(visitor, init);
	}

	std::optional<std::pair<RayHitInfo, std::reference_wrapper<Model>>> traceRay(const Ray& ray) const;

//private:
	std::unique_ptr<SceneNode> root;
};

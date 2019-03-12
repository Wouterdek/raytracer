#pragma once
#include "DynamicSceneNode.h"
#include <memory>
#include <functional>
#include <optional>

#include "scene/renderable/Scene.h"

class DynamicScene
{
public:
	DynamicScene();

	template<typename TState>
	std::optional<TState> walkDepthFirst(std::function<std::pair<TState, bool>(const DynamicSceneNode&, const TState&)> visitor, const TState& init) const
	{
		return this->root->walkDepthFirst(visitor, init);
	}

	Scene build() const;

//private:
	std::unique_ptr<DynamicSceneNode> root;
};

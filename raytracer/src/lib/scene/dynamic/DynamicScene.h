#pragma once
#include <memory>
#include <functional>
#include <optional>

#include "DynamicSceneNode.h"
#include "scene/renderable/Scene.h"
#include "utility/StatCollector.h"

class DynamicScene
{
public:
	DynamicScene();

	template<typename TState>
	std::optional<TState> walkDepthFirst(std::function<std::pair<TState, bool>(const DynamicSceneNode&, const TState&)> visitor, const TState& init) const
	{
		return this->root->walkDepthFirst(visitor, init);
	}

    DynamicScene soupifyScene(Statistics::Collector* stats = nullptr) const;
    Scene build(Statistics::Collector* stats = nullptr) const;

//private:
	std::unique_ptr<DynamicSceneNode> root;
	std::unique_ptr<IEnvironmentMaterial> environmentMaterial;
};

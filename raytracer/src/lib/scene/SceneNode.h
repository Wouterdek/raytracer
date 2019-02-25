#pragma once

#include <vector>
#include <memory>
#include <utility>
#include <optional>

#include "../math/Transformation.h"
#include "../model/Model.h"
#include "../camera/ICamera.h"
#include "../lamp/PointLamp.h"

class SceneNode
{
public:
	SceneNode();

	template<typename TState>
	std::optional<TState> walkDepthFirst(std::function<std::pair<TState, bool>(const SceneNode&, const TState&)> visitor, const TState& init) const
	{
		auto&[state, shouldContinue] = visitor(*this, init);
		if (!shouldContinue)
		{
			return state;
		}

		for (auto& child : children)
		{
			auto childState = child->walkDepthFirst(visitor, state);
			if (childState.has_value())
			{
				return *childState;
			}
		}

		return std::nullopt;
	}


//private:
	std::vector<std::unique_ptr<SceneNode>> children;
	Transformation transform;
	
	std::unique_ptr<Model> model;
	std::unique_ptr<ICamera> camera;
	std::unique_ptr<PointLamp> lamp;
};
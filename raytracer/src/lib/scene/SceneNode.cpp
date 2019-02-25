#include "SceneNode.h"

#include <optional>

SceneNode::SceneNode()
	: children(), transform(Transformation::IDENTITY)
{
}

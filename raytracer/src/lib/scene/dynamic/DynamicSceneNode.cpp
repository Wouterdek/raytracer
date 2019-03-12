#include "DynamicSceneNode.h"

#include <optional>

DynamicSceneNode::DynamicSceneNode()
	: children(), transform(Transformation::IDENTITY)
{
}

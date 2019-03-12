#pragma once
#include "math/Transformation.h"

template <typename T>
class SceneNode
{
public:
	SceneNode(Transformation transform, std::unique_ptr<T>&& data)
		: transform(std::move(transform)), data(std::move(data))
	{ }

	const Transformation& getTransform() const
	{
		return this->transform;
	}

	Transformation& getTransform()
	{
		return this->transform;
	}

	const T& getData() const
	{
		return *this->data;
	}

	T& getData()
	{
		return *this->data;
	}

private:
	Transformation transform;
	std::unique_ptr<T> data;
};

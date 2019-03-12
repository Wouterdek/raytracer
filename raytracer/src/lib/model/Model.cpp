#include "Model.h"

Model::Model(std::shared_ptr<IShape> shape, std::shared_ptr<IMaterial> material)
	: shape(std::move(shape)), material(std::move(material))
{ }

IShape& Model::getShape()
{
	return *this->shape;
}

const IShape& Model::getShape() const
{
	return *this->shape;
}

std::shared_ptr<IShape> Model::getShapePtr() const
{
	return this->shape;
}

const IMaterial& Model::getMaterial() const
{
	return *this->material;
}

Model* Model::cloneImpl()
{
	return new Model(*this);
}

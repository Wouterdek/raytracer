#pragma once

#include <memory>

#include "utility/ICloneable.h"
#include "shape/IShape.h"
#include "material/IMaterial.h"

class Model : public ICloneable<Model>
{
public:
	Model(std::shared_ptr<IShape> shape, std::shared_ptr<IMaterial> material);

	IShape& getShape()
    {
        return *this->shape;
    }

	const IShape& getShape() const
    {
        return *this->shape;
    }

	std::shared_ptr<IShape> getShapePtr() const
    {
        return this->shape;
    }

	const IMaterial& getMaterial() const
    {
        return *this->material;
    }

    std::shared_ptr<IMaterial> getMaterialPtr() const
    {
        return this->material;
    }

private:
	std::shared_ptr<IShape> shape;
	std::shared_ptr<IMaterial> material;

	Model* cloneImpl() const override
	{
        return new Model(*this);
    }
};

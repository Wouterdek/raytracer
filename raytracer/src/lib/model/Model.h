#pragma once

#include <memory>

#include "utility/ICloneable.h"
#include "shape/IShape.h"
#include "material/IMaterial.h"

class Model : public ICloneable<Model>
{
public:
	Model(std::shared_ptr<IShape> shape, std::shared_ptr<IMaterial> material);

	IShape& getShape();
	const IShape& getShape() const;
	std::shared_ptr<IShape> getShapePtr() const;
	const IMaterial& getMaterial() const;

private:
	std::shared_ptr<IShape> shape;
	std::shared_ptr<IMaterial> material;

	Model* cloneImpl() const override;
};

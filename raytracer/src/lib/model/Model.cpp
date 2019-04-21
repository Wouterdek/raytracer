#include "Model.h"

Model::Model(std::shared_ptr<IShape> shape, std::shared_ptr<IMaterial> material)
	: shape(std::move(shape)), material(std::move(material))
{ }


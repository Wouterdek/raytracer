#pragma once

#include <memory>

#include "../shape/IShape.h"
#include "../material/IMaterial.h"

class Model
{
public:

//private:
	std::shared_ptr<IShape> shape;
	std::shared_ptr<IMaterial> material;
};
#include "DiffuseMaterial.h"
#include "../scene/Scene.h"
#include "../math/Constants.h"

DiffuseMaterial::DiffuseMaterial()
{ }

RGB DiffuseMaterial::getColorFor(const RayHitInfo& hit, const Scene& scene, int depth)
{
	RGB ambient = this->diffuseColor.scale(this->diffuseIntensity).multiply(this->ambientColor.scale(this->ambientIntensity));

	RGB direct {};

	std::function<std::pair<Transformation, bool>(const SceneNode&, const Transformation&)> f = [&direct, &hit, &scene, this](const SceneNode& node, const Transformation& t)
	{
		Transformation cur = t.append(node.transform);

		if (node.lamp != nullptr)
		{
			//ITS LAMP TIME
			Point lampLocalPos{ 0, 0, 0 };
			Point lampPos = cur.transform(lampLocalPos);

			Point hitpoint = hit.getHitpoint();
			Vector3 objectToLamp = lampPos - hitpoint;
			double lampT = objectToLamp.norm();
			objectToLamp.normalize();

			Ray visibilityRay(hitpoint + (objectToLamp * 0.0001), objectToLamp);
			auto visibility = scene.traceRay(visibilityRay);
			bool isVisible = !visibility.has_value() || visibility->first.t > lampT;

			if(isVisible)
			{
				double angle = std::max(0.0f, hit.normal.dot(objectToLamp));
				auto cont = this->diffuseColor.scale(this->diffuseIntensity / PI).multiply(node.lamp->color.scale(node.lamp->intensity * angle));
				direct = direct.add(cont);
			}
		}
		return std::make_pair(cur, true);
	};
	scene.walkDepthFirst(f, Transformation::IDENTITY);

	return ambient.add(direct);
}

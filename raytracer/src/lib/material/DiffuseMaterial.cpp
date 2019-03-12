#include "DiffuseMaterial.h"
#include "scene/dynamic/DynamicScene.h"
#include "math/Constants.h"
#include "math/Transformation.h"

DiffuseMaterial::DiffuseMaterial()
{ }

RGB DiffuseMaterial::getColorFor(const RayHitInfo& hit, const Scene& scene, int depth) const
{
	RGB ambient = this->diffuseColor.scale(this->diffuseIntensity).multiply(this->ambientColor.scale(this->ambientIntensity));

	RGB direct {};

	for(const auto& light : scene.getLights())
	{
		Point lampLocalPos{ 0, 0, 0 };
		Point lampPos = light.getTransform().transform(lampLocalPos);

		Point hitpoint = hit.getHitpoint();
		Vector3 objectToLamp = lampPos - hitpoint;
		double lampT = objectToLamp.norm();
		objectToLamp.normalize();

		Ray visibilityRay(hitpoint + (objectToLamp * 0.0001), objectToLamp);
		auto visibility = scene.traceRay(visibilityRay);
		bool isVisible = !visibility.has_value() || visibility->getGeometryInfo().t > lampT;

		if (isVisible)
		{
			double angle = std::max(0.0f, hit.normal.dot(objectToLamp));
			auto cont = this->diffuseColor.scale(this->diffuseIntensity / PI).multiply(light.getData().color.scale(light.getData().intensity * angle));
			direct = direct.add(cont);
		}
	}

	return ambient.add(direct);
}

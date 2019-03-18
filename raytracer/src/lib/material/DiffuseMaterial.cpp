#include "DiffuseMaterial.h"
#include "scene/dynamic/DynamicScene.h"
#include "math/Constants.h"
#include "math/Transformation.h"
#include "scene/renderable/SceneRayHitInfo.h"

DiffuseMaterial::DiffuseMaterial()
{ }

RGB DiffuseMaterial::getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const
{
	auto diffuseColor = this->diffuseColor;
	if(this->albedoMap != nullptr)
	{
		float x = abs(fmod(hit.texCoord.x(), 1.0f));
		float y = abs(fmod(hit.texCoord.y(), 1.0f));
		diffuseColor = this->albedoMap->get(x * this->albedoMap->getWidth(), y * this->albedoMap->getHeight());
	}

	auto normal = hit.normal;
	if(this->normalMap != nullptr)
	{
		float x = abs(fmod(hit.texCoord.x(), 1.0f));
		float y = abs(fmod(hit.texCoord.y(), 1.0f));
		auto normalColor = this->normalMap->get(x * this->albedoMap->getWidth(), y * this->albedoMap->getHeight());
		normal = Vector3(normalColor.getRed(), normalColor.getGreen(), normalColor.getBlue()); //TODO: this is wrong (object space/world space)
	}

	RGB ambient = diffuseColor.scale(this->diffuseIntensity).multiply(this->ambientColor.scale(this->ambientIntensity));

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
		bool isVisible = !visibility.has_value() || visibility->t > lampT;

		if (isVisible)
		{
			double angle = std::max(0.0f, normal.dot(objectToLamp));
			auto cont = diffuseColor.scale(this->diffuseIntensity / PI).multiply(light.getData().color.scale(light.getData().intensity * angle));
			direct = direct.add(cont);
		}
	}

	return ambient.add(direct);
}

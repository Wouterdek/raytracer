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
		auto mapNormal = Vector3(normalColor.getRed(), normalColor.getGreen(), normalColor.getBlue());
		normal = hit.getModelNode().getTransform().transformNormal(mapNormal);
	}

	RGB ambient = (diffuseColor * this->diffuseIntensity).multiply(this->ambientColor * this->ambientIntensity);

	RGB direct {};

	Point hitpoint = hit.getHitpoint();

	for(const auto& light : scene.getAreaLights())
	{
		const int sampleCount = 16;
		RGB contrib{};
		for(int i = 0; i < sampleCount; i++)//todo
		{
			auto lampPoint = light->generateRandomPoint();
			Vector3 objectToLamp = lampPoint - hitpoint;
			double lampT = objectToLamp.norm();
			objectToLamp.normalize();

			Ray visibilityRay(hitpoint + (objectToLamp * 0.0001f), objectToLamp);
			auto visibility = scene.traceRay(visibilityRay);
			bool isVisible = !visibility.has_value() || visibility->t > lampT;

			if (isVisible)
			{
				double lampAngle = std::max(0.0f, light->getNormal().dot(-objectToLamp));
				double geometricFactor = lampAngle / pow(lampT, 2);
				double angle = std::max(0.0f, normal.dot(objectToLamp));
				auto lampRadiance = light->color * (light->intensity * angle) * geometricFactor * light->getSurfaceArea();
				auto cont = (diffuseColor * (this->diffuseIntensity / PI)).multiply(lampRadiance);
				contrib = contrib.add(cont);
			}
		}
		direct = direct.add(contrib.divide(sampleCount));
	}

	for(const auto& light : scene.getPointLights())
	{
		Vector3 objectToLamp = light->pos - hitpoint;
		double lampT = objectToLamp.norm();
		objectToLamp.normalize();

		Ray visibilityRay(hitpoint + (objectToLamp * 0.0001), objectToLamp);
		auto visibility = scene.traceRay(visibilityRay);
		bool isVisible = !visibility.has_value() || visibility->t > lampT;

		if (isVisible)
		{
			double angle = std::max(0.0f, normal.dot(objectToLamp));
			auto cont = (diffuseColor * (this->diffuseIntensity / PI)).multiply(light->color * (light->intensity * angle));
			direct = direct.add(cont);
		}
	}

	return ambient.add(direct);
}

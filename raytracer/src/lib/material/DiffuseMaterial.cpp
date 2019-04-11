#include "DiffuseMaterial.h"
#include "scene/dynamic/DynamicScene.h"
#include "math/Constants.h"
#include "math/Transformation.h"
#include "math/OrthonormalBasis.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include <random>

thread_local std::random_device randDev;
std::uniform_real_distribution<float> randAngle(-90, 90);

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

	//RGB ambient = (diffuseColor * this->diffuseIntensity).multiply(this->ambientColor * this->ambientIntensity);

	RGB direct {};

	Point hitpoint = hit.getHitpoint();

	for(const auto& light : scene.getAreaLights())
	{
		const int sampleCount = 4;
		RGB contrib{};
		for(int i = 0; i < sampleCount; i++)
		{
			auto lampPoint = light->generateStratifiedJitteredRandomPoint(sampleCount, i);
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
				contrib = contrib.add(lampRadiance);
			}
		}
		direct += contrib.divide(sampleCount);
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
			direct += light->color * (light->intensity * angle);
		}
	}

	// Indirect lighting
	RGB indirect {};
	int maxDepth = 3;
	if(depth < maxDepth)
	{
		auto angleA = randAngle(randDev);
		auto angleB = randAngle(randDev);
		OrthonormalBasis basis(hit.normal);
		auto transform = Transformation::rotate(basis.getV(), angleB).append(Transformation::rotate(basis.getU(), angleA));
		Vector3 direction = transform.transform(hit.normal);

		Ray ray(hitpoint + (direction * 0.0001f), direction);
		auto nextHit = scene.traceRay(ray);
		if(nextHit.has_value())
		{
			double angle = std::max(0.0f, normal.dot(direction));
			indirect += nextHit->getModelNode().getData().getMaterial().getColorFor(*nextHit, scene, depth + 1).scale(angle);
		}
	}

	return diffuseColor.multiply(direct.scale(this->diffuseIntensity / PI) + indirect.scale(this->diffuseIntensity * 2));
}

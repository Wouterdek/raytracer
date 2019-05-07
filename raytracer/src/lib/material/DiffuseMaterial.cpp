#include "DiffuseMaterial.h"
#include "scene/dynamic/DynamicScene.h"
#include "math/Constants.h"
#include "math/Transformation.h"
#include "math/OrthonormalBasis.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "NormalMapSampler.h"
#include <random>

namespace {
    thread_local std::random_device randDev;
    std::uniform_real_distribution<float> randAngle(-90, 90);
};

DiffuseMaterial::DiffuseMaterial() = default;

Vector3 sampleBounceDirection(const Vector3& surfaceNormal)
{
    auto angleA = randAngle(randDev);
    auto angleB = randAngle(randDev);
    OrthonormalBasis basis(surfaceNormal);
    auto transform = Transformation::rotate(basis.getV(), angleB).append(Transformation::rotate(basis.getU(), angleA));
    return transform.transform(surfaceNormal);
}

RGB brdf(const RGB& lIn, const Vector3& surfaceNormal, const Vector3& outDir)
{
    double angle = std::max(0.0f, surfaceNormal.dot(outDir));
    return lIn.scale(angle);
}

RGB DiffuseMaterial::getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const
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
        auto mapNormal = sample_normal_map(hit, *this->normalMap);
		normal = hit.getModelNode().getTransform().transformNormal(mapNormal);
	}

	const auto& photonMap = scene.getPhotonMap();
	if(photonMap.has_value())
    {
        std::vector<const Photon*> photons(10);
        photonMap->getElementsNearestTo(hit.getHitpoint(), photons.size(), photons);

        RGB value {};
        float maxDistSqr = 0;
        for(const Photon* photon : photons)
        {
            if(hit.ray.getDirection().dot(-photon->incomingDir) < 0)
            {
                continue;
            }

            auto squaredDist = (photon->getPosition() - hit.getHitpoint()).squaredNorm();
            maxDistSqr = std::max(maxDistSqr, squaredDist);
            /*auto val = 1.0/(sqrt(squaredDist) + 1);
            value += RGB(photon->isCaustic ? 0: val, photon->isCaustic ? val : 0, 0);*/
            value += brdf(photon->energy * 200.0, normal, -photon->incomingDir);
        }
        return diffuseColor.multiply(value.scale(this->diffuseIntensity).divide(PI*maxDistSqr));
    }

	RGB direct {};

	Point hitpoint = hit.getHitpoint();

	for(const auto& light : scene.getAreaLights())
	{
		const int sampleCount = 1;
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
	int maxDepth = 4;
	if(depth < maxDepth)
	{
		Vector3 direction = sampleBounceDirection(hit.normal);

		Ray ray(hitpoint + (direction * 0.0001f), direction);
		auto nextHit = scene.traceRay(ray);
		if(nextHit.has_value())
		{

			auto lIn = nextHit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*nextHit, scene, depth + 1);
			indirect += brdf(lIn, normal, direction);
		}
	}

	return diffuseColor.multiply(direct.scale(this->diffuseIntensity / PI) + indirect.scale(this->diffuseIntensity * 2));
}

std::tuple<Vector3, RGB, float> DiffuseMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const {
    Vector3 direction = sampleBounceDirection(hit.normal);
    return std::make_tuple(direction, brdf(incomingEnergy, hit.normal, direction), 1.0f);
}

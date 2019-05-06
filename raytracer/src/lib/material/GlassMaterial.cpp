#include "math/Constants.h"
#include "GlassMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/Vector3.h"
#include <random>

namespace {
    thread_local std::random_device randDevice;
    std::uniform_real_distribution<float> randDist(0, 1);
};

Vector3 sampleTransportDirection(Vector3& normal, Vector3 incomingDir, double ior)
{
    incomingDir = -incomingDir;
    Vector3 reflectionRayDir = -incomingDir + ((2.0 * normal.dot(incomingDir)) * normal);
    /*RGB reflection{};
    {
		Vector3 reflectionRayDir = -incomingDir + ((2.0 * normal.dot(incomingDir)) * normal);
		Ray reflectionRay(hitpoint + reflectionRayDir * 0.001, reflectionRayDir);
		auto reflectionHit = scene.traceRay(reflectionRay);
		if (reflectionHit.has_value()) {
			reflection = reflectionHit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*reflectionHit, scene, depth + 1)
				* abs(normal.dot(reflectionRayDir));
		}
    }*/

    // Check for total internal reflection
    double nDotWo = normal.dot(incomingDir);
    auto incomingAngle = nDotWo; //assuming wo and n are normalized
    bool isInternalRay = nDotWo < 0;
    auto curIor = ior;
    if(isInternalRay)
    {
        curIor = 1.0 / curIor;
    }
    double cosTransmissionSqr = 1.0 - ((1.0 - pow(incomingAngle, 2.0)) / pow(curIor, 2.0));
    bool tir = cosTransmissionSqr < 0; // total internal reflection

    if(tir)
    {
        return reflectionRayDir;
        //return RGB::BLACK;
    }

    if(isInternalRay)
    {
        nDotWo = -nDotWo;
        incomingAngle = -incomingAngle;
        normal = -normal;
        cosTransmissionSqr = 1.0 - ((1.0 - pow(incomingAngle, 2.0)) / pow(curIor, 2.0));
    }
    double cosTransmission = sqrt(cosTransmissionSqr);
    Vector3 transmittedRayDir = (-incomingDir / curIor) - (cosTransmission - incomingAngle / curIor) * normal;

    auto reflectionWeight = abs(normal.dot(reflectionRayDir));
    auto transmissionWeight = (1.0/pow(curIor, 2.0));
    auto totalWeight = reflectionWeight + transmissionWeight; // = 1.0 ?

    if(randDist(randDevice) * totalWeight < reflectionWeight)
    {
        return reflectionRayDir;
    }
    else
    {
        return transmittedRayDir;
    }
}

RGB GlassMaterial::getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const
{
    if(depth > 5){
        return RGB::BLACK;
    }

    auto normal = hit.normal;
    auto hitpoint = hit.getHitpoint();
    auto direction = sampleTransportDirection(normal, hit.ray.getDirection(), ior);
    Ray ray(hitpoint + (direction * 0.001), direction);

    auto nestedHit = scene.traceRay(ray);
    if (nestedHit.has_value()) {
        auto lIn = nestedHit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*nestedHit, scene, depth + 1);
        return lIn; //TODO: is weight correct?
    }

    return RGB::BLACK;


	/*Ray transmittedRay(hitpoint + transmittedRayDir * 0.001, transmittedRayDir);

	auto transmissionHit = scene.traceRay(transmittedRay);
	RGB transmission = RGB::BLACK;
	if (transmissionHit.has_value()) {
		transmission =
                transmissionHit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*transmissionHit, scene,
                                                                                                depth + 1)
			* (1.0/pow(curIor, 2.0));
	}*/
	//return transmission;// +reflection;
}

std::tuple<Vector3, RGB, float> GlassMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    Vector3 normal = hit.normal;
    auto direction = sampleTransportDirection(normal, hit.ray.getDirection(), ior);
    return std::make_tuple(direction, incomingEnergy, 0.0); // TODO: is weight correct?
}

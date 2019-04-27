#include "math/Constants.h"
#include "GlassMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/Vector3.h"

RGB GlassMaterial::getColorFor(const SceneRayHitInfo &hit, const Scene &scene, int depth) const
{
    if(depth > 5){
        return RGB::BLACK;
    }

    auto normal = hit.normal;
    auto hitpoint = hit.getHitpoint();
    Vector3 incomingDir = -hit.ray.getDirection();

	/*RGB reflection{};
    {
		Vector3 reflectionRayDir = -incomingDir + ((2.0 * normal.dot(incomingDir)) * normal);
		Ray reflectionRay(hitpoint + reflectionRayDir * 0.001, reflectionRayDir);
		auto reflectionHit = scene.traceRay(reflectionRay);
		if (reflectionHit.has_value()) {
			reflection = reflectionHit->getModelNode().getData().getMaterial().getColorFor(*reflectionHit, scene, depth + 1)
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
	bool tir = cosTransmissionSqr < 0;

	if(tir)
	{
		//return reflection;
		return RGB::BLACK;
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
	Ray transmittedRay(hitpoint + transmittedRayDir * 0.001, transmittedRayDir);

	auto transmissionHit = scene.traceRay(transmittedRay);
	RGB transmission = RGB::BLACK;
	if (transmissionHit.has_value()) {
		transmission = transmissionHit->getModelNode().getData().getMaterial().getColorFor(*transmissionHit, scene, depth + 1)
			* (1.0/pow(curIor, 2.0));
	}
	return transmission;// +reflection;
}

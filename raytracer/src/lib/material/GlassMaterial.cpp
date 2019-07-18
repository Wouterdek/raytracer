#include "math/Constants.h"
#include "GlassMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/Vector3.h"
#include <random>
#include <math/Triangle.h>

namespace {
    thread_local std::random_device randDevice;
    std::uniform_real_distribution<float> randDist(0, 1);
};

Vector3 sampleTransportDirection(Vector3& normal, Vector3 incomingDir, double ior, bool& isReflectionDirection)
{
    auto objToIncoming = -incomingDir;
    Vector3 reflectionRayDir = -objToIncoming + ((2.0 * normal.dot(objToIncoming)) * normal);

    // Check for total internal reflection
    double nDotWo = normal.dot(objToIncoming);
    auto incomingAngle = nDotWo; //assuming wo and n are normalized //Careful: this is cos of angle between normal and Wo, which could be >90 (if internal ray)
    bool isInternalRay = nDotWo < 0;
    auto curRelIor = ior / 1.0; //Assuming 1.0 for air
    if(isInternalRay)
    {
        curRelIor = 1.0 / curRelIor;
    }
    double cosTransmissionSqr = 1.0 - ((1.0 - pow(incomingAngle, 2.0)) / pow(curRelIor, 2.0)); //incomingAngle could actually be -incomingAngle, but this doesn't matter as we square it anyway
    bool tir = cosTransmissionSqr < 0; // total internal reflection

    if(tir)
    {
        isReflectionDirection = true;
        return reflectionRayDir;
    }

    if(isInternalRay)
    {
        nDotWo = -nDotWo;
        incomingAngle = -incomingAngle;
        normal = -normal;
    }
    double cosRefractAngle = sqrt(cosTransmissionSqr);

    Vector3 transmittedRayDir = (incomingDir / curRelIor) - (cosRefractAngle - (incomingAngle / curRelIor)) * normal;

    auto rs = ((curRelIor * nDotWo) - cosRefractAngle) / ((curRelIor * nDotWo) + cosRefractAngle);
    auto rp = (nDotWo - (curRelIor * cosRefractAngle)) / (nDotWo + (curRelIor * cosRefractAngle));
    auto kr = ((rs * rs) + (rp * rp)) / 2.0;
    auto reflectionWeight = kr;
    auto transmissionWeight = 1.0 - kr;
    auto totalWeight = reflectionWeight + transmissionWeight;

    if(randDist(randDevice) * totalWeight < reflectionWeight)
    {
        isReflectionDirection = true;
        return reflectionRayDir;
    }
    else
    {
        isReflectionDirection = false;
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
    bool isReflection;
    auto direction = sampleTransportDirection(normal, hit.ray.getDirection(), ior, isReflection);
    bool isInternalRay = normal.dot(direction) < 0;

    Ray ray(hitpoint + (direction * 0.001), direction);

    auto result = scene.traceRay(ray);

    const AreaLight* lightHit = nullptr;
    double bestT = result.has_value() ? result->t : 1E99;
    for(const auto& light : scene.getAreaLights())
    {
        Triangle::TriangleIntersection intersection;
        bool hasIntersection = Triangle::intersect(ray, light->a, light->b, light->c, intersection);
        if(hasIntersection && bestT > intersection.t){
            lightHit = &*light;
            bestT = intersection.t;
        }
    }

    RGB val = RGB::BLACK;
    if(lightHit != nullptr)
    {
        val = lightHit->color * lightHit->intensity;
    }
    else if(result.has_value())
    {
        val = result->getModelNode().getData().getMaterial().getTotalRadianceTowards(*result, scene, depth + 1);
    }

    if(isInternalRay)
    {
        val = val.multiply(this->color.pow(attenuationStrength*bestT));
    }

    return val;
}

std::tuple<Vector3, RGB, float> GlassMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    Vector3 normal = hit.normal;
    bool isReflection;
    auto direction = sampleTransportDirection(normal, hit.ray.getDirection(), ior, isReflection);
    return std::make_tuple(direction, incomingEnergy, 0.0); // TODO: is weight correct?
}

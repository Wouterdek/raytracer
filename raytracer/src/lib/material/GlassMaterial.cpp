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
#pragma pack(push, 1)
struct TransportMetaData
{
    float reflectionWeight = 0;
    float t = 0;
    Vector3 reflectedRayDir;
    Vector3 transmittedRayDir;
    AreaLight* lightHit = nullptr;
};
#pragma pack(pop)
void GlassMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.specularity = 1.0;
    
    auto* meta = transport.metadata.tryRead<TransportMetaData>();
    if(meta == nullptr)
    {
        meta = transport.metadata.alloc<TransportMetaData>();

        //HYBRID PHOTON
        if(ctx.curI != 0 && (ctx.path[ctx.curI-1].specularity < 0.8 && ctx.path[ctx.curI-1].type == TransportType::bounce))
        {
            transport.isEmissive = true;
            transport.pathTerminationChance = 1.0;
            meta->reflectionWeight = NAN;
            return;
        }
        //END

        transport.specularity = 1.0;
        transport.isEmissive = false;
        transport.pathTerminationChance = 0.1;

        auto& normal = transport.hit.normal;
        auto& incomingDir = transport.hit.ray.getDirection();
        auto objToIncoming = -incomingDir;
        meta->reflectedRayDir = -objToIncoming + ((2.0 * normal.dot(objToIncoming)) * normal);

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
            meta->reflectionWeight = 1.0;
            transport.type = TransportType::bounce;
            transport.transportDirection = meta->reflectedRayDir;
            return; //TODO: lights should be checked here too
        }
        else
        {
            if(isInternalRay)
            {
                nDotWo = -nDotWo;
                incomingAngle = -incomingAngle;
                normal = -normal;
            }
            double cosRefractAngle = sqrt(cosTransmissionSqr);

            meta->transmittedRayDir = (incomingDir / curRelIor) - (cosRefractAngle - (incomingAngle / curRelIor)) * normal;

            auto rs = ((curRelIor * nDotWo) - cosRefractAngle) / ((curRelIor * nDotWo) + cosRefractAngle);
            auto rp = (nDotWo - (curRelIor * cosRefractAngle)) / (nDotWo + (curRelIor * cosRefractAngle));
            auto kr = ((rs * rs) + (rp * rp)) / 2.0;
            auto reflectionWeight = kr;
            auto transmissionWeight = 1.0 - kr;
            auto totalWeight = reflectionWeight + transmissionWeight;
            meta->reflectionWeight = reflectionWeight / totalWeight;
        }
    }

    if(randDist(randDevice) < meta->reflectionWeight)
    {
        transport.type = TransportType::bounce;
        transport.transportDirection = meta->reflectedRayDir;
    }
    else
    {
        transport.type = TransportType::transmit;
        transport.transportDirection = meta->transmittedRayDir;
    }

    // Check if there are any lights in this direction
    Point hitpoint = transport.hit.getHitpoint();
    Ray ray(hitpoint+(transport.transportDirection*.0001), transport.transportDirection);
    auto result = ctx.scene.traceRay(ray);

    double bestT = result.has_value() ? result->t : 1E99;
    for(const auto& light : ctx.scene.getAreaLights())
    {
        Triangle::TriangleIntersection intersection;
        bool hasIntersection = Triangle::intersect(ray, light->a, light->b, light->c, intersection);
        if(hasIntersection && bestT > intersection.t){
            meta->lightHit = &*light;
            bestT = intersection.t;
        }
    }
    meta->t = bestT;

    if(meta->lightHit == nullptr)
    {
        ctx.nextHit = result;
        transport.isEmissive = false;
        transport.pathTerminationChance = 0.1;
    }
    else
    {
        transport.isEmissive = true;
        transport.pathTerminationChance = 1.0;
    }
}

RGB GlassMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    auto* meta = curNode.metadata.tryRead<TransportMetaData>();

    //HYBRID PHOTON
    if(isnanf(meta->reflectionWeight))
    {
        return RGB::BLACK;
    }
    //END

    auto normal = curNode.hit.normal;
    auto direction = curNode.transportDirection;
    bool isInternalRay = normal.dot(direction) < 0;

    RGB val = incomingEnergy;
    if(meta->lightHit != nullptr)
    {
        val = meta->lightHit->color * meta->lightHit->intensity;
    }

    if(isInternalRay)
    {
        val = val.multiply(this->color.pow(attenuationStrength*meta->t));
    }

    return val;
}


Vector3 sampleTransportDirection(Vector3& normal, const Vector3& incomingDir, double ior, bool& isReflectionDirection)
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
    bool isInternalRay = normal.dot(-hit.ray.getDirection()) < 0;

    auto energy = incomingEnergy;
    if(isInternalRay)
    {
        auto dist = (hit.ray.getOrigin() - hit.getHitpoint()).norm();
        energy = incomingEnergy.multiply(this->color.pow(attenuationStrength*dist));
    }

    bool isReflection;
    auto direction = sampleTransportDirection(normal, hit.ray.getDirection(), ior, isReflection);

    return std::make_tuple(direction, energy, 0.0);
}

bool GlassMaterial::hasVariance(const SceneRayHitInfo &hit, const Scene &scene) const
{
    return true;
}

#include "math/Constants.h"
#include "GlassMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/Vector3.h"
#include "math/FastRandom.h"
#include <math/Triangle.h>

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
        if(ctx.scene.getPhotonMapMode() == PhotonMapMode::caustics && ctx.curI != 0 && (ctx.path[ctx.curI-1].specularity < 0.8 && ctx.path[ctx.curI-1].type == TransportType::bounce))
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
            meta->t = NAN;

            transport.type = TransportType::bounce;
            transport.transportDirection = meta->reflectedRayDir;
            return; //TODO: should lights be checked here too?
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

    if(Rand::unit() < meta->reflectionWeight)
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
        auto t = std::isnan(meta->t) ? curNode.hit.t : meta->t;
        val = val.multiply(this->color.pow(attenuationStrength*t));
    }

    return val;
}

Vector3 samplePhotonDirection(Vector3& normal, const Vector3& incomingDir, double ior, bool& isReflectionDirection)
{
    Vector3 objToIncoming = -incomingDir;
    Vector3 reflectedRayDir = -objToIncoming + ((2.0 * normal.dot(objToIncoming)) * normal);
    Vector3 transmittedRayDir;
    double reflectionWeight;

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
        return reflectedRayDir; //TODO: should lights be checked here too?
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

        transmittedRayDir = (incomingDir / curRelIor) - (cosRefractAngle - (incomingAngle / curRelIor)) * normal;

        auto rs = ((curRelIor * nDotWo) - cosRefractAngle) / ((curRelIor * nDotWo) + cosRefractAngle);
        auto rp = (nDotWo - (curRelIor * cosRefractAngle)) / (nDotWo + (curRelIor * cosRefractAngle));
        auto kr = ((rs * rs) + (rp * rp)) / 2.0;
        reflectionWeight = kr;
        auto transmissionWeight = 1.0 - kr;
        auto totalWeight = reflectionWeight + transmissionWeight;
        reflectionWeight = reflectionWeight / totalWeight;
    }

    if(Rand::unit() < reflectionWeight)
    {
        return reflectedRayDir;
    }
    else
    {
        return transmittedRayDir;
    }
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

    if(Rand::unit() * totalWeight < reflectionWeight)
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

std::tuple<Vector3, RGB, float> GlassMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    Vector3 normal = hit.normal;
    bool isInternalRay = normal.dot(hit.ray.getDirection()) < 0;
    //bool isInternalRay = normal.dot(hit.ray.getDirection()) > 0;

    bool isReflection;
    auto direction = samplePhotonDirection(normal, hit.ray.getDirection(), ior, isReflection);

    auto energy = incomingEnergy;
    if(isInternalRay)
    {
        auto dist = hit.t;
        energy = incomingEnergy.multiply(this->color.pow(attenuationStrength*dist));
    }

    return std::make_tuple(direction, energy, 0.0);
}

bool GlassMaterial::hasVariance(const SceneRayHitInfo &hit, const Scene &scene) const
{
    return true;
}

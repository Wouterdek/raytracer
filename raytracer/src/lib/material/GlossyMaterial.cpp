#include "GlossyMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"
#include "math/Triangle.h"
#include "NormalMapSampler.h"
#include "math/Sampler.h"
#include <cmath>

GlossyMaterial::GlossyMaterial() = default;

struct TransportMetaData
{
    AreaLight* lightHit = nullptr;
    Vector3 perfectReflectionDir;
};

void GlossyMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    auto* meta = transport.metadata.tryRead<TransportMetaData>();
    if(meta == nullptr)
    {
        meta = transport.metadata.alloc<TransportMetaData>();

        auto& incomingDir = transport.hit.ray.getDirection();
        auto& normal = transport.hit.normal;
        if(this->normalMap != nullptr)
        {
            auto mapNormal = sample_normal_map(transport.hit, *this->normalMap);
            normal = transport.hit.getModelNode().getTransform().transformNormal(mapNormal);
        }
        meta->perfectReflectionDir = incomingDir + ((2.0*normal.dot(-incomingDir))*normal);
    }

    Vector3 sampleDir = meta->perfectReflectionDir;
    auto offset = sampleUniformCircle(roughness);
    OrthonormalBasis sampleSpace(sampleDir);
    sampleDir += sampleSpace.getU() * offset.x();
    sampleDir += sampleSpace.getV() * offset.y();

    transport.transportDirection = sampleDir;
    transport.specularity = 1.0 - this->roughness;
    transport.type = TransportType::bounce;

    // Check if there are any lights in this direction
    Point hitpoint = transport.hit.getHitpoint();
    Ray ray(hitpoint+(sampleDir*.001), sampleDir);
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

    if(meta->lightHit == nullptr)
    {
        ctx.nextHit = result;
        transport.isEmissive = false;
        transport.pathTerminationChance = 0.2;
    }
    else
    {
        transport.isEmissive = true;
        transport.pathTerminationChance = 1.0;
    }
}

RGB GlossyMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    auto* meta = curNode.metadata.tryRead<TransportMetaData>();
    RGB radiance = incomingEnergy;
    if(meta->lightHit != nullptr)
    {
        radiance = meta->lightHit->color * meta->lightHit->intensity;
    }

    return radiance.multiply(color);
}


Vector3 sampleReflectionDir(Vector3 normal, Vector3 incomingDir, float roughness)
{
    Vector3 sampleDir = incomingDir + ((2.0*normal.dot(-incomingDir))*normal);

    auto offset = sampleUniformCircle(roughness);
    OrthonormalBasis sampleSpace(sampleDir);
    sampleDir += sampleSpace.getU() * offset.x();
    sampleDir += sampleSpace.getV() * offset.y();

    return sampleDir;
}

std::tuple<Vector3, RGB, float> GlossyMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    Vector3 normal = hit.normal;
    if(this->normalMap != nullptr)
    {
        auto mapNormal = sample_normal_map(hit, *this->normalMap);
        normal = hit.getModelNode().getTransform().transformNormal(mapNormal);
    }

    Vector3 direction = sampleReflectionDir(normal, hit.ray.getDirection(), this->roughness);
    //TODO: is this energy weight correct?
    //TODO: is using roughness for diffuse parameter correct?
    return std::make_tuple(direction, incomingEnergy, this->roughness);
}

bool GlossyMaterial::hasVariance(const SceneRayHitInfo &hit, const Scene &scene) const
{
    return this->roughness > 0.0f;
}

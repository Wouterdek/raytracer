#include "GlossyMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"
#include "math/Triangle.h"
#include "NormalMapSampler.h"
#include "math/Sampler.h"
#include "VNDFGGXSampler.h"

GlossyMaterial::GlossyMaterial() = default;

struct TransportMetaData
{
    AreaLight* lightHit = nullptr;
    Vector3 normal;
};

Vector3 getPerfectReflectionDir(const Vector3& normal, const Vector3& incomingDir)
{
    return incomingDir + ((2.0*normal.dot(-incomingDir))*normal);
}

void GlossyMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.specularity = 1.0f - this->roughness;
    transport.type = TransportType::bounce;

    auto* meta = transport.metadata.tryRead<TransportMetaData>();
    if(meta == nullptr)
    {
        meta = transport.metadata.alloc<TransportMetaData>();

        const auto& incomingDir = transport.hit.ray.getDirection();
        meta->normal = transport.hit.normal;
        if(this->normalMap != nullptr)
        {
            meta->normal = sample_normal_map(transport.hit, *this->normalMap);
        }
    }

    Vector3 microfacetNormal = VNDFGGXSampler::sample(meta->normal, -transport.hit.ray.getDirection(), roughness);
    microfacetNormal.normalize();
    Vector3 sampleDir = getPerfectReflectionDir(microfacetNormal, transport.hit.ray.getDirection());

    transport.transportDirection = sampleDir;

    if(sampleDir.dot(transport.hit.normal) < 0)
    {
        // sampleDir points to wrong side of geometry. Assume self-collide with zero-contribution
        transport.isEmissive = true;
        transport.pathTerminationChance = 1.0;
        return;
    }

    // Check if there are any lights in this direction
    Point hitpoint = transport.hit.getHitpoint();
    Ray ray(hitpoint+(sampleDir*.001), sampleDir);
    auto result = ctx.scene.traceRay(ray);

    meta->lightHit = nullptr;
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
        auto lightEnergy = meta->lightHit->color * meta->lightHit->intensity;
        auto lightIrradiance = lightEnergy.divide(meta->lightHit->getSurfaceArea());
        radiance = lightIrradiance.divide(2); //TODO: check this
    }
    return radiance.multiply(color);
}

std::tuple<Vector3, RGB, float> GlossyMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    Vector3 normal = hit.normal;
    if(this->normalMap != nullptr)
    {
        normal = sample_normal_map(hit, *this->normalMap);
    }

    Vector3 microfacetNormal = VNDFGGXSampler::sample(normal, -hit.ray.getDirection(), roughness);
    microfacetNormal.normalize();
    Vector3 sampleDir = getPerfectReflectionDir(microfacetNormal, hit.ray.getDirection());

    //TODO: is this energy weight correct?
    //TODO: is using roughness for diffuse parameter correct?
    return std::make_tuple(sampleDir, incomingEnergy, this->roughness);
}

bool GlossyMaterial::hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const
{
    return this->roughness > 0.0f;
}

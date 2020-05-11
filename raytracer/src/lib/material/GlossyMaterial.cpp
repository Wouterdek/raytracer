#include "GlossyMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"
#include "math/Triangle.h"
#include "NormalMapSampler.h"
#include "math/Sampler.h"
#include "VNDFGGXSampler.h"
#include "NextEventEstimation.h"

GlossyMaterial::GlossyMaterial() = default;

struct TransportMetaData
{
    AreaLight* lightHit = nullptr; // Used when NEE is disabled
    Vector3 normal;
    RGB neeRadiance;
    bool isNEERay = false;
};

Vector3 getPerfectReflectionDir(const Vector3& normal, const Vector3& incomingDir)
{
    return incomingDir + ((2.0*normal.dot(-incomingDir))*normal);
}

float ggx(const float roughness, const Vector3& v, const Vector3& n, const Vector3& m) //I/O vector, normal, micronormal
{
    auto theta_v = std::acos(v.dot(n));
    if(v.dot(m) / v.dot((n)) > 0.0f)
    {
        auto tan_theta = std::tan(theta_v);
        return 2.0f / (1.0f + std::sqrt(1.0f + (roughness*roughness) * (tan_theta * tan_theta)));
    }
    else
    {
        return 0.0f;
    }
}

float brdf(const float roughness, const Vector3& i, const Vector3& o, const Vector3& n, const Vector3& m)
{
    float g = ggx(roughness, i, n, m) * ggx(roughness, o, n, m);
    if(g == 0.0f)
    {
        return 0.0f;
    }

    float d = 0.0f;
    if(m.dot(n) > 0)
    {
        float theta_m = std::acos(m.dot(n));
        float aSqr = roughness * roughness;
        float tan_theta_m = std::tan(theta_m);
        float cos_theta_m = std::cos(theta_m);
        float cos_theta_m_sqr = cos_theta_m * cos_theta_m;
        float x = aSqr + tan_theta_m * tan_theta_m;
        d = aSqr / ((float)M_PI * cos_theta_m_sqr * cos_theta_m_sqr * x * x);
    }
    else
    {
        return 0.0f;
    }

    return g * d / (4.0f * std::abs(i.dot(n)) * std::abs(o.dot(n)));
}

void GlossyMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    Point hitpoint = transport.hit.getHitpoint();
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

    bool neeEnabled = this->roughness > 0;
    bool useNEE = neeEnabled && Rand::unit() > 0.5; // Do NEE 50% of the time if roughness > 0
    meta->isNEERay = useNEE;
    meta->lightHit = nullptr;
    if(useNEE)
    {
        //TODO: has a bug, but looks close enough for now.
        auto radiance = NextEventEstimation::sample(ctx.scene, hitpoint, meta->normal, ctx.sampleI, ctx.sampleCount, transport.transportDirection);
        auto microNormal = (-transport.hit.ray.getDirection() + transport.transportDirection).normalized();
        double angle = std::max(0.0f, microNormal.dot(transport.transportDirection));
        auto f = brdf(roughness, -transport.hit.ray.getDirection(), transport.transportDirection, meta->normal, microNormal);
        meta->neeRadiance = radiance * angle * f;
        transport.isEmissive = true;
        transport.pathTerminationChance = 1.0;
    }
    else
    {
        meta->neeRadiance = RGB::BLACK;

        auto randSquare = sampleUniformStratifiedSquare(ctx.sampleCount, ctx.sampleI);
        Vector3 microfacetNormal = VNDFGGXSampler::sample(meta->normal, -transport.hit.ray.getDirection(), roughness, randSquare.x(), randSquare.y());
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
        Ray ray(hitpoint+(transport.transportDirection*.001), transport.transportDirection);
        auto result = ctx.scene.traceRay(ray);

        if(neeEnabled)
        {
            bool isLightHit = false;
            for(const auto& light : ctx.scene.getAreaLights())
            {
                Triangle::TriangleIntersection intersection;
                bool hasIntersection = Triangle::intersect(ray, light->a, light->b, light->c, intersection);
                if(hasIntersection && (!result.has_value() || result->t > intersection.t)){
                    isLightHit = true;
                    break;
                }
            }

            //Black sample on light hit to prevent double counting of lights in NEE and non-NEE
            if(isLightHit)
            {
                transport.isEmissive = true;
                transport.pathTerminationChance = 1.0;
                //meta->neeRadiance is set to black above
                meta->isNEERay = true; //Tells shading to use neeLighting value
            }
            else
            {
                transport.isEmissive = false;
                transport.pathTerminationChance = 0.2;
                ctx.nextHit = result;
            }
        }
        else
        {
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
    }
}

RGB GlossyMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    auto* meta = curNode.metadata.tryRead<TransportMetaData>();
    bool neeEnabled = this->roughness > 0;
    RGB radiance = incomingEnergy;

    if(neeEnabled)
    {
        if(meta->isNEERay)
        {
            radiance = meta->neeRadiance;
        }
        radiance = radiance.scale(2.0f); // *2 because monte carlo sum with 50% chance of NEE and non-NEE
    }
    else
    {
        if(meta->lightHit != nullptr)
        {
            auto lightEnergy = meta->lightHit->color * meta->lightHit->intensity;
            auto lightIrradiance = lightEnergy.divide(meta->lightHit->getSurfaceArea());
            radiance = lightIrradiance.divide(2); //TODO: check this
        }
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

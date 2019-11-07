#include <math/Sampler.h>
#include "DiffuseMaterial.h"
#include "scene/dynamic/DynamicScene.h"
#include "math/Constants.h"
#include "math/Transformation.h"
#include "math/OrthonormalBasis.h"
#include "math/Triangle.h"
#include "math/FastRandom.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "NormalMapSampler.h"

DiffuseMaterial::DiffuseMaterial() = default;

// Return radiance from light to hitpoint
RGB neePointLight(const PointLight& light, const Scene& scene, const Point& hitpoint, const Vector3& normal, /* OUT */ Vector3& lightDirection)
{
    Vector3 objectToLamp = light.pos - hitpoint;
    auto lampT = objectToLamp.norm();
    objectToLamp.normalize();
    lightDirection = objectToLamp;

    Ray visibilityRay(hitpoint + (objectToLamp * 0.0001f), objectToLamp);
    bool isVisible = !scene.testVisibility(visibilityRay, lampT).has_value();

    if (isVisible)
    {
        //auto angle = std::max(0.0f, normal.dot(objectToLamp));
        auto geometricFactor = 1.0f / (4.0f * PI * pow(lampT, 2));

        return light.color * (light.intensity * geometricFactor);
    }
    return RGB::BLACK;
}

// Return radiance*theta_lamp from light to hitpoint, picking a stratified random sample point as representative for the entire light
RGB neeAreaLight(const AreaLight& light, const Scene& scene, const Point& hitpoint, const Vector3& normal, int sampleI, int sampleCount, /* OUT */ Vector3& lightDirection)
{
    auto lampPoint = light.generateStratifiedJitteredRandomPoint(sampleCount, sampleI);
    Vector3 objectToLamp = lampPoint - hitpoint;
    auto lampT = objectToLamp.norm();
    objectToLamp.normalize();
    lightDirection = objectToLamp;

    Ray visibilityRay(hitpoint + (objectToLamp * 0.0001f), objectToLamp);
    bool isVisible = !scene.testVisibility(visibilityRay, lampT).has_value();
    if (isVisible)
    {
        auto lightEnergy = light.color * light.intensity;
        auto lightIrradiance = lightEnergy.divide(light.getSurfaceArea());
        auto lightRadiance = lightIrradiance.divide(PI);

        //auto surfaceAngle = std::max(0.0f, normal.dot(objectToLamp));
        auto lampAngle = std::max(0.0f, light.getNormal().dot(-objectToLamp));
        auto geometricFactor = lampAngle / std::pow(lampT, 2.0f);

        auto irradianceFromLamp = lightRadiance * geometricFactor;
        return irradianceFromLamp * light.getSurfaceArea();
    }
    return RGB::BLACK;
}

RGB doNextEventEstimation(const Scene& scene, const Point& hitpoint, const Vector3& normal, int sampleI, int sampleCount, /* OUT */ Vector3& lightDirection)
{
    bool hasAreaLights = !scene.getAreaLights().empty();
    bool hasPointLights = !scene.getPointLights().empty();

    /*
     * probability of choosing point light:
     *        HPL
     *        Y   N
     * HAL Y 0.5 0.0
     *     N 1.0 0.0
     */
    float pointLightProbability = std::max((hasPointLights * 1.0f) - (hasAreaLights * 0.5f), 0.0f);

    if(Rand::unit() < pointLightProbability) // Choose point light
    {
        float chosenLightProbability = 1.0f/scene.getPointLights().size();
        auto lightIndex = Rand::intInRange(scene.getPointLights().size()-1);
        const auto& light = *scene.getPointLights()[lightIndex].get();

        return neePointLight(light, scene, hitpoint, normal, lightDirection).divide(pointLightProbability * chosenLightProbability);
    }
    else if(hasAreaLights) // Choose area light
    {
        float chosenLightProbability = 1.0f/scene.getAreaLights().size();
        auto lightIndex = Rand::intInRange(scene.getAreaLights().size()-1);
        const auto& light = *scene.getAreaLights()[lightIndex].get();

        return neeAreaLight(light, scene, hitpoint, normal, sampleI, sampleCount, lightDirection).divide((1.0f-pointLightProbability) * chosenLightProbability);
    }
    else
    {
        return RGB::BLACK;
    }
}

struct TransportMetaData
{
    RGB directLighting;
    RGB photonLighting;
    bool photonLightingIsSet;
    bool isPhotonMapRay = false;
    bool isNEERay = false;
};

void DiffuseMaterial::sampleTransport(TransportBuildContext& ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.specularity = 0.0f;
    transport.type = TransportType::bounce;

    auto* meta = transport.metadata.readOrAlloc<TransportMetaData>();
    meta->isPhotonMapRay = false;

    auto normal = transport.hit.normal;
    if(this->normalMap != nullptr)
    {
        auto mapNormal = sample_normal_map(transport.hit, *this->normalMap);
        normal = transport.hit.getModelNode().getTransform().transformNormal(mapNormal);
    }

    bool readFromPhotonMap = false;
    if(ctx.scene.getPhotonMapMode() == PhotonMapMode::full)
    {
        int curDiffuseI = 0;
        for(int i = 0; i < ctx.curI; i++)
        {
            if(ctx.path[i].specularity < 0.8)
            {
                curDiffuseI++;
            }
        }
        if(curDiffuseI >= ctx.scene.getPhotonMapDepth())
        {
            readFromPhotonMap = true;
        }
    }

    if(readFromPhotonMap)
    {
        transport.pathTerminationChance = 1.0f;
        transport.isEmissive = true;
        meta->isPhotonMapRay = true;

        if(!meta->photonLightingIsSet)
        {
            const auto& photonMap = ctx.scene.getPhotonMap();
            std::vector<const Photon*> photons(20);

            auto hitpoint = transport.hit.getHitpoint();
            auto dir = -transport.hit.ray.getDirection();
            auto [nbPhotonsFound, maxDist] = photonMap->getElementsNearestTo(hitpoint, photons.size(), 1E9, [dir](const Photon& photon){
                return dir.dot(photon.surfaceNormal) >= 0;
            }, photons);

            RGB value {};
            for(unsigned int i = 0; i < nbPhotonsFound; i++)
            {
                value += photons[i]->energy;
            }
            meta->photonLighting = value.scale(this->diffuseIntensity).divide(PI*maxDist*maxDist).divide(PI);
            meta->photonLightingIsSet = true;
        }
    }
    else
    {
        transport.pathTerminationChance = 0.1f;
        transport.isEmissive = false;

        bool useNextEventEstimation = Rand::unit() > 0.5f;
        if(useNextEventEstimation)
        {
            meta->directLighting = doNextEventEstimation(ctx.scene, transport.hit.getHitpoint(), normal, ctx.sampleI, ctx.sampleCount, transport.transportDirection);
            transport.pathTerminationChance = 1.0f;
            transport.isEmissive = true;
            meta->isNEERay = true;
        }
        else
        {
            OrthonormalBasis basis(normal);
            //auto localDir = sampleStratifiedCosineWeightedHemisphere(std::sqrt(ctx.sampleCount), ctx.sampleI, 1.0);
            auto localDir = mapSampleToCosineWeightedHemisphere(Rand::unit(), Rand::unit(), 1.0);
            transport.transportDirection = (basis.getU() * localDir.x()) + (basis.getV() * localDir.y()) + (basis.getW() * localDir.z());

            meta->isNEERay = false;

            if(ctx.scene.getPhotonMapMode() == PhotonMapMode::caustics)
            {
                ctx.nextNodeCallback = [&transport, &ctx, meta, normal](){
                    if(ctx.getCurNode().specularity > 0.8)
                    {
                        transport.pathTerminationChance = 1.0f;
                        transport.isEmissive = true;

                        if(!meta->photonLightingIsSet)
                        {
                            const auto& photonMap = ctx.scene.getPhotonMap();
                            std::vector<const Photon*> photons(20);

                            auto dir = -transport.hit.ray.getDirection();
                            auto [nbPhotonsFound, maxDist] = photonMap->getElementsNearestTo(transport.hit.getHitpoint(), photons.size(), 1E9, [dir](const Photon& photon)
                            {
                                return dir.dot(photon.surfaceNormal) >= 0;
                            }, photons);

                            RGB value {};
                            for(unsigned int i = 0; i < nbPhotonsFound; ++i)
                            {
                                value += photons[i]->energy;
                            }
                            meta->photonLighting = value.divide(PI*(maxDist*maxDist)).divide(PI);
                            meta->photonLightingIsSet = true;
                        }

                        meta->isPhotonMapRay = true;
                    }
                };
            }
        }
    }
}

RGB DiffuseMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode& curNode, const RGB &incomingEnergy) const
{
    auto* meta = curNode.metadata.tryRead<TransportMetaData>();

    auto diffuseColor = this->diffuseColor;
    if(this->albedoMap != nullptr)
    {
        float x = abs(fmod(curNode.hit.texCoord.x(), 1.0f));
        float y = abs(fmod(curNode.hit.texCoord.y(), 1.0f));
        diffuseColor = this->albedoMap->get(x * this->albedoMap->getWidth(), y * this->albedoMap->getHeight());
    }

    if(meta->isPhotonMapRay)
    {
        auto out = diffuseColor.multiply(meta->photonLighting);

        if(scene.getPhotonMapMode() == PhotonMapMode::caustics)
        {
            //TODO: do we need *4 here? 2 for hemispherical sampling, 2 for separate NEE rays? (or *2*pi ?)
            out = out.scale(2);
            //out = out.scale(2) * PI;
        }

        return out;
    }
    else
    {
        auto normal = curNode.hit.normal;
        if(this->normalMap != nullptr)
        {
            auto mapNormal = sample_normal_map(curNode.hit, *this->normalMap);
            normal = curNode.hit.getModelNode().getTransform().transformNormal(mapNormal);
        }

        //Applying MC to integration requires multiplying by 1/pdf().
        //In hemispherical integration the pdf() = 1/hemisphere_area
        // => multiply by hemisphere area = 2pi * pi/2 = pi^2
        //In light area integration the pdf() = 1/total_area
        // => multiply by total light area (is done above in doNextEventEstimation())
        //The diffuse BRDF has a correction term of 1/pi to be energy conservant.
        //Finally, applying MC to (direct + indirect) with a 50% chance for each term means each term should be multiplied by 2.

        double angle = std::max(0.0f, normal.dot(curNode.transportDirection));
        RGB value;
        if(meta->isNEERay)
        {
            RGB direct = meta->directLighting.scale(angle);
            value = direct.scale(this->diffuseIntensity / PI).scale(2);
        }
        else
        {
            RGB indirect = incomingEnergy.scale(angle);
            value = indirect.scale(this->diffuseIntensity).scale(2);
        }
        return diffuseColor.multiply(value);
    }
}


Vector3 sampleBounceDirection(const Vector3& surfaceNormal)
{
    OrthonormalBasis basis(surfaceNormal);
    auto localDir = mapSampleToCosineWeightedHemisphere(Rand::unit(), Rand::unit(), 1.0);
    return (basis.getU() * localDir.x()) + (basis.getV() * localDir.y()) + (basis.getW() * localDir.z());
}

std::tuple<Vector3, RGB, float> DiffuseMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
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

    Vector3 direction = sampleBounceDirection(normal);

    return std::make_tuple(direction, diffuseColor.multiply(incomingEnergy), 1.0f);
}

bool DiffuseMaterial::hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const
{
    if(scene.getPhotonMapMode() == PhotonMapMode::full)
    {
        int curDiffuseI = 0;
        for(int i = 0; i < curI; i++)
        {
            if(path[i].specularity < 0.8)
            {
                curDiffuseI++;
            }
        }
        if(curDiffuseI >= scene.getPhotonMapDepth())
        {
            return false;
        }
    }

    return true;
}
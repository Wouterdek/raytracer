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

RGB doNextEventEstimation(const Scene& scene, const Point& hitpoint, const Vector3& normal, int sampleI, int sampleCount, /* OUT */ Vector3& lightDirection)
{
    auto totalLightCount = scene.getAreaLights().size() + scene.getPointLights().size();
    if(totalLightCount == 0)
    {
        return RGB::BLACK;
    }

    auto lightIndex = Rand::intInRange(totalLightCount-1);

    double totalArea = 0;
    for(const auto& light : scene.getAreaLights())
    {
        totalArea += light->getSurfaceArea();
    }

    //TODO: this is wrong if the area lights are of different sizes. To have equal probability for any point,
    //      the probability of picking any light should be proportional to its area.
    //      Combining point and area light sources should be treating both as terms in a sum that is to be importance sampled
    //      based on factors such as number of lights, intensity, area, distance, ...
    //  e.g. 3 levels: pick light type, pick light, pick light point
    //      the probability of each choice at each level is multiplied and used to divide the result
    if(lightIndex < scene.getAreaLights().size())
    {
        const auto& light = scene.getAreaLights()[lightIndex];
        auto lampPoint = light->generateStratifiedJitteredRandomPoint(sampleCount, sampleI);
        Vector3 objectToLamp = lampPoint - hitpoint;
        auto lampT = objectToLamp.norm();
        objectToLamp.normalize();
        lightDirection = objectToLamp;

        Ray visibilityRay(hitpoint + (objectToLamp * 0.0001f), objectToLamp);
        bool isVisible = !scene.testVisibility(visibilityRay, lampT).has_value();
        if (isVisible)
        {
            auto lampAngle = std::max(0.0f, light->getNormal().dot(-objectToLamp));
            auto angle = std::max(0.0f, normal.dot(objectToLamp));
            auto geometricFactor = (angle * lampAngle) / pow(lampT, 2);

            auto lE = light->color * light->intensity;

            auto lampRadiance = lE * geometricFactor * totalArea;
            return lampRadiance;
        }
    }
    else
    {
        const auto& light = scene.getPointLights()[lightIndex - scene.getAreaLights().size()];
        Vector3 objectToLamp = light->pos - hitpoint;
        auto lampT = objectToLamp.norm();
        objectToLamp.normalize();
        lightDirection = objectToLamp;

        Ray visibilityRay(hitpoint + (objectToLamp * 0.0001f), objectToLamp);
        bool isVisible = !scene.testVisibility(visibilityRay, lampT).has_value();

        if (isVisible)
        {
            auto angle = std::max(0.0f, normal.dot(objectToLamp));
            auto geometricFactor = angle / pow(lampT, 2);
            //TODO: should compensate for sampling point lights (see above)
            return light->color * (light->intensity * geometricFactor);
        }
    }

    return RGB::BLACK;
}

struct TransportMetaData
{
    RGB directLighting;
    RGB photonLighting;
    bool photonLightingIsSet;
    bool isPhotonMapRay = false;
    bool isNEERay = false;
};

RGB brdf(const RGB& lIn, const Vector3& surfaceNormal, const Vector3& outDir)
{
    double angle = std::max(0.0f, surfaceNormal.dot(outDir));
    return lIn.scale(angle);
}

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

            auto dir = -transport.hit.ray.getDirection();
            auto [nbPhotonsFound, maxDist] = photonMap->getElementsNearestTo(transport.hit.getHitpoint(), photons.size(), 1E9, [dir](const Photon& photon){
                return dir.dot(photon.surfaceNormal) >= 0;
            }, photons);

            RGB value {};
            for(int i = 0; i < nbPhotonsFound; i++)
            {
                const Photon* photon = photons[i];
                value += brdf(photon->energy, normal, -photon->incomingDir);
            }
            meta->photonLighting = value.scale(this->diffuseIntensity).divide(PI*(maxDist*maxDist));
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
            auto angleA = Rand::floatInRange(-90, 90);
            auto angleB = Rand::floatInRange(-90, 90);
            OrthonormalBasis basis(normal);
            auto transform = Transformation::rotate(basis.getV(), angleB).append(Transformation::rotate(basis.getU(), angleA));
            transport.transportDirection = transform.transform(normal);
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
                                const Photon* photon = photons[i];
                                value += brdf(photon->energy, normal, -photon->incomingDir);
                            }
                            meta->photonLighting = value.divide(PI*(maxDist*maxDist));
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

        RGB value;
        if(meta->isNEERay)
        {
            RGB direct = brdf(meta->directLighting, normal, curNode.transportDirection);
            value = direct.scale(this->diffuseIntensity / PI).scale(2);
            //value = direct.scale(this->diffuseIntensity / (PI * PI)).scale(2);
        }
        else
        {
            RGB indirect = brdf(incomingEnergy, normal, curNode.transportDirection);
            value = indirect.scale(this->diffuseIntensity * 2).scale(2);
            //value = indirect.scale(this->diffuseIntensity * PI).scale(2);
        }
        return diffuseColor.multiply(value);
    }
}


Vector3 sampleBounceDirection(const Vector3& surfaceNormal)
{
    auto angleA = Rand::floatInRange(-90, 90);
    auto angleB = Rand::floatInRange(-90, 90);
    OrthonormalBasis basis(surfaceNormal);
    auto transform = Transformation::rotate(basis.getV(), angleB).append(Transformation::rotate(basis.getU(), angleA));
    return transform.transform(surfaceNormal);
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
    return std::make_tuple(direction, diffuseColor.multiply(brdf(incomingEnergy, hit.normal, direction)), 1.0f);
}

bool DiffuseMaterial::hasVariance(const SceneRayHitInfo &hit, const Scene &scene) const
{
    return scene.getPhotonMapMode() != PhotonMapMode::full;
}
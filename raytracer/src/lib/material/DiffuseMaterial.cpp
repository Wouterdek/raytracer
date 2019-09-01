#include "DiffuseMaterial.h"
#include "scene/dynamic/DynamicScene.h"
#include "math/Constants.h"
#include "math/Transformation.h"
#include "math/OrthonormalBasis.h"
#include "math/Triangle.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "NormalMapSampler.h"
#include <random>

namespace {
    thread_local std::random_device randDev;
    std::uniform_real_distribution<float> randAngle(-90, 90);
    std::uniform_real_distribution<float> randUnit(0, 1);
};

DiffuseMaterial::DiffuseMaterial() = default;

RGB doNextEventEstimation(const Scene& scene, const Point& hitpoint, const Vector3& normal, int sampleI, int sampleCount, /* OUT */ Vector3& lightDirection)
{
    auto totalLightCount = scene.getAreaLights().size() + scene.getPointLights().size();
    if(totalLightCount == 0)
    {
        return RGB::BLACK;
    }

    auto lightIndex = std::uniform_int_distribution<int>(0, totalLightCount-1)(randDev);

    double totalArea = 0;
    for(const auto& light : scene.getAreaLights())
    {
        totalArea += light.get()->getSurfaceArea();
    }

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
            return light->color * (light->intensity * angle);
        }
    }

    return RGB::BLACK;
}

struct TransportMetaData
{
    RGB directLighting;
    bool directLightingIsSet;
    bool isNEERay = false;
    bool isPartialPhotonMapRay = false;
};

RGB brdf(const RGB& lIn, const Vector3& surfaceNormal, const Vector3& outDir)
{
    double angle = std::max(0.0f, surfaceNormal.dot(outDir));
    return lIn.scale(angle);
}

void DiffuseMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.specularity = 0.0f;
    transport.type = TransportType::bounce;

    auto* meta = transport.metadata.readOrAlloc<TransportMetaData>();
    meta->isPartialPhotonMapRay = false;

    auto normal = transport.hit.normal;
    if(this->normalMap != nullptr)
    {
        auto mapNormal = sample_normal_map(transport.hit, *this->normalMap);
        normal = transport.hit.getModelNode().getTransform().transformNormal(mapNormal);
    }

    if(false && ctx.scene.getPhotonMap().has_value())
    {
        transport.pathTerminationChance = 1.0f;
        transport.isEmissive = true;

        const auto& photonMap = ctx.scene.getPhotonMap();
        std::vector<const Photon*> photons(20);

        auto dir = -transport.hit.ray.getDirection();
        auto [nbPhotonsFound, maxDist] = photonMap->getElementsNearestTo(transport.hit.getHitpoint(), photons.size(), 1, [dir](const Photon& photon){
#ifdef PHOTONMAP_CAUSTIC_ONLY
            return dir.dot(photon.surfaceNormal) >= 0 && photon.isCaustic;
#else
            return dir.dot(photon.surfaceNormal) >= 0;
#endif
        }, photons);

        RGB value {};
        for(int i = 0; i < nbPhotonsFound; i++)
        {
            const Photon* photon = photons[i];
            value += brdf(photon->energy, normal, -photon->incomingDir);
        }
        meta->directLighting = value.scale(this->diffuseIntensity).divide(PI*(maxDist*maxDist));
        meta->directLightingIsSet = true;
    }
    else
    {
        transport.pathTerminationChance = 0.2f;
        transport.isEmissive = false;

        bool useNextEventEstimation = randUnit(randDev) > 0.5f;
        if(useNextEventEstimation)
        {
            meta->directLighting = doNextEventEstimation(ctx.scene, transport.hit.getHitpoint(), normal, ctx.sampleI, ctx.sampleCount, transport.transportDirection);
            transport.pathTerminationChance = 1.0f;
            transport.isEmissive = true;
            meta->isNEERay = true;
        }
        else
        {
            auto angleA = randAngle(randDev);
            auto angleB = randAngle(randDev);
            OrthonormalBasis basis(normal);
            auto transform = Transformation::rotate(basis.getV(), angleB).append(Transformation::rotate(basis.getU(), angleA));
            transport.transportDirection = transform.transform(normal);
            meta->isNEERay = false;

            if(ctx.scene.getPhotonMap().has_value())
            {
                ctx.nextNodeCallback = [&transport, &ctx, meta, normal](){
                    if(ctx.getCurNode().specularity > 0.8)
                    {
                        transport.pathTerminationChance = 1.0f;
                        transport.isEmissive = true;

                        if(!meta->directLightingIsSet)
                        {
                            const auto& photonMap = ctx.scene.getPhotonMap();
                            std::vector<const Photon*> photons(20);

                            auto dir = -transport.hit.ray.getDirection();
                            auto [nbPhotonsFound, maxDist] = photonMap->getElementsNearestTo(transport.hit.getHitpoint(), photons.size(), 1, [dir](const Photon& photon)
                            {
                                return dir.dot(photon.surfaceNormal) >= 0;
                            }, photons);

                            RGB value {};
                            for(int i = 0; i < nbPhotonsFound; i++)
                            {
                                const Photon* photon = photons[i];
                                value += brdf(photon->energy, normal, -photon->incomingDir);
                            }
                            meta->directLighting = value.divide(PI*(maxDist*maxDist));
                            meta->directLightingIsSet = true;
                        }

                        meta->isPartialPhotonMapRay = true;
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

    if(scene.getPhotonMap().has_value() && meta->isPartialPhotonMapRay)
    {
        return diffuseColor.multiply(meta->directLighting).scale(2);
    }
    else
    {
        auto normal = curNode.hit.normal;
        if(this->normalMap != nullptr)
        {
            auto mapNormal = sample_normal_map(curNode.hit, *this->normalMap);
            normal = curNode.hit.getModelNode().getTransform().transformNormal(mapNormal);
        }

        RGB value;
        if(meta->isNEERay)
        {
            RGB direct = brdf(meta->directLighting, normal, curNode.transportDirection);
            value = direct.scale(this->diffuseIntensity / PI).scale(2);
        }
        else
        {
            RGB indirect = brdf(incomingEnergy, normal, curNode.transportDirection);
            value = indirect.scale(this->diffuseIntensity * 2).scale(2);
        }
        return diffuseColor.multiply(value);
    }
}


Vector3 sampleBounceDirection(const Vector3& surfaceNormal)
{
    auto angleA = randAngle(randDev);
    auto angleB = randAngle(randDev);
    OrthonormalBasis basis(surfaceNormal);
    auto transform = Transformation::rotate(basis.getV(), angleB).append(Transformation::rotate(basis.getU(), angleA));
    return transform.transform(surfaceNormal);
}

RGB DiffuseMaterial::getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const
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

#if defined(PHOTONMAP_FULL) || defined(PHOTONMAP_CAUSTIC_ONLY)
	const auto& photonMap = scene.getPhotonMap();
	if(photonMap.has_value())
    {
        std::vector<const Photon*> photons(200);

        auto dir = -hit.ray.getDirection();
        auto [nbPhotonsFound, maxDist] = photonMap->getElementsNearestTo(hit.getHitpoint(), photons.size(), 1, [dir](const Photon& photon){
#ifdef PHOTONMAP_CAUSTIC_ONLY
            return dir.dot(photon.surfaceNormal) >= 0 && photon.isCaustic;
#else
            return dir.dot(photon.surfaceNormal) >= 0;
#endif
        }, photons);

        RGB value {};
        for(int i = 0; i < nbPhotonsFound; i++)
        {
            const Photon* photon = photons[i];
            /*auto val = 1.0/(sqrt(squaredDist) + 1);
            value += RGB(photon->isCaustic ? 0: val, photon->isCaustic ? val : 0, 0);*/
            value += brdf(photon->energy, normal, -photon->incomingDir);
            //value += photon->energy;
        }
        return diffuseColor.multiply(value.scale(this->diffuseIntensity).divide(PI*(maxDist*maxDist)));
    }
#endif

    Point hitpoint = hit.getHitpoint();


//#define DIFFUSE_NAIVE
#ifdef DIFFUSE_NAIVE
    RGB value {};
    const int maxDepth = 4;
    if(depth < maxDepth)
    {
        Vector3 direction = sampleBounceDirection(hit.normal);
        Ray ray(hitpoint + (direction * 0.0001f), direction);
        auto nextHit = scene.traceRay(ray);
        auto bestT = nextHit.has_value() ? nextHit->t : 1E99;

        bool lightHit = false;
        for(const auto& light : scene.getAreaLights())
        {
            Triangle::TriangleIntersection intersection;
            bool intersects = Triangle::intersect(ray, light->a, light->b, light->c, intersection);

            if(intersects && intersection.t < bestT)
            {
                auto lampRadiance = light->color * light->intensity;
                value += brdf(lampRadiance, normal, direction);
                lightHit = true;
                break;
            }
        }

        if(!lightHit && nextHit.has_value())
        {
            auto lIn = nextHit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*nextHit, scene, depth + 1);
            value += brdf(lIn, normal, direction);
        }
    }

    return diffuseColor.multiply(value.scale(this->diffuseIntensity * 2));
#else
    RGB direct {};

    for(const auto& light : scene.getAreaLights())
    {
        const int sampleCount = 1;
        RGB contrib{};
        for(int i = 0; i < sampleCount; i++)
        {
            auto lampPoint = light->generateStratifiedJitteredRandomPoint(sampleCount, i);
            Vector3 objectToLamp = lampPoint - hitpoint;
            auto lampT = objectToLamp.norm();
            objectToLamp.normalize();

            Ray visibilityRay(hitpoint + (objectToLamp * 0.0001f), objectToLamp);
            bool isVisible = !scene.testVisibility(visibilityRay, lampT).has_value();
            if (isVisible)
            {
                auto lampAngle = std::max(0.0f, light->getNormal().dot(-objectToLamp));
                auto angle = std::max(0.0f, normal.dot(objectToLamp));
                auto geometricFactor = (angle * lampAngle) / pow(lampT, 2);

                auto lE = light->color * light->intensity;

                auto lampRadiance = lE * geometricFactor * light->getSurfaceArea();
                contrib = contrib.add(lampRadiance);
            }
        }
        direct += contrib.divide(sampleCount);
    }

    for(const auto& light : scene.getPointLights())
    {
        Vector3 objectToLamp = light->pos - hitpoint;
        auto lampT = objectToLamp.norm();
        objectToLamp.normalize();

        Ray visibilityRay(hitpoint + (objectToLamp * 0.0001f), objectToLamp);
        bool isVisible = !scene.testVisibility(visibilityRay, lampT).has_value();

        if (isVisible)
        {
            auto angle = std::max(0.0f, normal.dot(objectToLamp));
            direct += light->color * (light->intensity * angle);
        }
    }

    // Indirect lighting
    RGB indirect {};
    const int maxDepth = 4;
    const int branchingFactor = 1;
    if(depth < maxDepth)
    {
        for(int i = 0; i < branchingFactor; i++)
        {
            Vector3 direction = sampleBounceDirection(hit.normal);

            Ray ray(hitpoint + (direction * 0.0001f), direction);
            auto nextHit = scene.traceRay(ray);
            if(nextHit.has_value())
            {
                auto lIn = nextHit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*nextHit, scene, depth + 1);
                indirect += brdf(lIn, normal, direction);
            }
        }
        indirect = indirect.divide(branchingFactor);
    }

//#define DIFFUSE_SINGLE_LEVEL
#ifdef DIFFUSE_SINGLE_LEVEL
    const auto renderDepth = 3;
	if(depth == renderDepth)
    {
        return diffuseColor.multiply(direct.scale(this->diffuseIntensity / PI));
    }else{
        return diffuseColor.multiply(indirect.scale(this->diffuseIntensity * 2));
	}
#else
    return diffuseColor.multiply(direct.scale(this->diffuseIntensity / PI) + indirect.scale(this->diffuseIntensity * 2));
#endif
#endif
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
    return true || !scene.getPhotonMap().has_value();
}
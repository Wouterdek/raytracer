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
#include "NextEventEstimation.h"

DiffuseMaterial::DiffuseMaterial() = default;

struct TransportMetaData
{
    RGB directLighting;
    RGB photonLighting;
    bool photonLightingIsSet;
    bool isPhotonMapRay = false;
    bool isNEERay = false;
};

static float photonrays_lookup_insert(const PhotonRay* elem, std::vector<const PhotonRay*>& resultsList, unsigned int count, const Point& target, const Vector3& n)
{
    auto distF = [](const PhotonRay* r, const Point& p, const Vector3& n)
    {
        // Find intersection of line with local plane
        auto l0 = (r->direction.cross(r->moment));
        Vector3 intersection = l0 + (r->direction * (p - l0).dot(n)/(r->direction.dot(n)));
        Vector3 vect = intersection - p;

        // Find closest point on line
        //Vector3 vect = (p.cross(r->direction) - r->moment);
        return vect;
    };
    auto begin = resultsList.begin() + (resultsList.size() - count);
    auto it = std::lower_bound(begin, resultsList.end(), elem, [&target, &n, distF=distF](const PhotonRay* c1, const PhotonRay* c2){
        auto dist1 = c1 == nullptr ? 1E99 : distF(c1, target, n).squaredNorm();
        auto dist2 = c2 == nullptr ? 1E99 : distF(c2, target, n).squaredNorm();
        return dist1 < dist2;
    });
    if(*it != elem)
    {
        resultsList.insert(it, elem);
        resultsList.pop_back();
    }
    auto lastElemPtr = resultsList.back();
    if(lastElemPtr == nullptr)
    {
        return std::numeric_limits<float>::max();
    }
    return distF(lastElemPtr, target, n).norm();
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
        normal = sample_normal_map(transport.hit, *this->normalMap);
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

    if(ctx.scene.getPhotonMapMode() == PhotonMapMode::rays)
    {
        //TODO: non-caustics aren't enabled/supported/working

        transport.pathTerminationChance = 1.0f;
        transport.isEmissive = true;
        meta->isPhotonMapRay = true;

        auto maxPhotons = std::min(20lu, ctx.scene.getPhotonRayMap()->size());
        std::vector<const PhotonRay*> closestRays(maxPhotons);
        float maxDist = INFINITY;
        for(const auto& ray : ctx.scene.getPhotonRayMap().value())
        {
            maxDist = photonrays_lookup_insert(&ray, closestRays, closestRays.size(), transport.hit.getHitpoint(), normal);
        }

        RGB value {};
        for(unsigned int i = 0; i < closestRays.size(); ++i)
        {
            value += closestRays[i]->energy;
        }
        meta->photonLighting = value.scale(this->diffuseIntensity).divide(PI*maxDist*maxDist).divide(PI);

        meta->photonLightingIsSet = true;
    }
    else if(readFromPhotonMap)
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
            meta->directLighting = NextEventEstimation::sample(ctx.scene, transport.hit.getHitpoint(), normal, ctx.sampleI, ctx.sampleCount, transport.transportDirection);
            transport.pathTerminationChance = 1.0f;
            transport.isEmissive = true;
            meta->isNEERay = true;
        }
        else
        {
            OrthonormalBasis basis(normal);
            auto localDir = sampleStratifiedCosineWeightedHemisphere(std::sqrt(ctx.sampleCount), ctx.sampleI, 1.0);
            //auto localDir = mapSampleToCosineWeightedHemisphere(Rand::unit(), Rand::unit(), 1.0);
            transport.transportDirection = (basis.getU() * localDir.x()) + (basis.getV() * localDir.y()) + (basis.getW() * localDir.z());
            assert(!transport.transportDirection.hasNaN());
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
        float x = fmodf(curNode.hit.texCoord.x(), 1.0f);
        float y = fmodf(curNode.hit.texCoord.y(), 1.0f);
        if(x < 0){ x = 1.0f + x;}
        if(y < 0){ y = 1.0f + y;}
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
            normal = sample_normal_map(curNode.hit, *this->normalMap);
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
        normal = sample_normal_map(hit, *this->normalMap);
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
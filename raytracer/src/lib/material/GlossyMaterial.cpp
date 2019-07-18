#include "GlossyMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"
#include "math/Triangle.h"
#include "NormalMapSampler.h"
#include "math/UniformSampler.h"
#include <random>
#include <cmath>

GlossyMaterial::GlossyMaterial() = default;

Vector3 sampleReflectionDir(Vector3 normal, Vector3 incomingDir, float roughness)
{
    Vector3 sampleDir = incomingDir + ((2.0*normal.dot(-incomingDir))*normal);

    auto offset = sampleUniformCircle(roughness);
    OrthonormalBasis sampleSpace(sampleDir);
    sampleDir += sampleSpace.getU() * offset.x();
    sampleDir += sampleSpace.getV() * offset.y();

    return sampleDir;
}

RGB GlossyMaterial::getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const
{
    int maxDepth = 4;
    if(depth >= maxDepth){
        return RGB::BLACK;
    }

    Vector3 normal = hit.normal;
    if(this->normalMap != nullptr)
    {
        auto mapNormal = sample_normal_map(hit, *this->normalMap);
        normal = hit.getModelNode().getTransform().transformNormal(mapNormal);
    }

    Point hitpoint = hit.getHitpoint();

    RGB radiance {};
    const int branchingFactor = 1;
    for(int i = 0; i < branchingFactor; ++i)
    {
        Vector3 sampleDir = sampleReflectionDir(normal, hit.ray.getDirection(), this->roughness);

        Ray ray(hitpoint+(sampleDir*.001), sampleDir);
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

        //TODO: non-zero roughness should include point lights

        //TEST
        /*const auto renderDepth = 3;
        if(depth == renderDepth)
        {
            return RGB::BLACK;
        }else{
            if(lightHit != nullptr){
                return lightHit->color * lightHit->intensity;
            }

            if(result.has_value()){
                return result->getModelNode().getData().getMaterial().getTotalRadianceTowards(*result, scene, depth + 1);
            }

            return RGB::BLACK;
        }*/
        //TEST

        if(lightHit != nullptr){
            radiance += lightHit->color * lightHit->intensity;
            continue;
        }

        if(result.has_value()){
            radiance += result->getModelNode().getData().getMaterial().getTotalRadianceTowards(*result, scene, depth + 1);
            continue;
        }
    }

    return radiance.divide(branchingFactor).multiply(color);
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




#include "GlossyMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"
#include "math/Triangle.h"
#include "NormalMapSampler.h"
#include "math/CircularUniformSampler.h"
#include <random>
#include <cmath>

GlossyMaterial::GlossyMaterial() = default;

RGB GlossyMaterial::getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const
{
    int maxDepth = 4;
    if(depth >= maxDepth){
        return RGB::BLACK;
    }

    Vector3 incomingDir = hit.ray.getDirection();
    Vector3 normal = hit.normal;
    if(this->normalMap != nullptr)
    {
        auto mapNormal = sample_normal_map(hit, *this->normalMap);
        normal = hit.getModelNode().getTransform().transformNormal(mapNormal);
    }

    Vector3 sampleDir = incomingDir + ((2.0*normal.dot(-incomingDir))*normal);

    auto offset = sampleUniformCircle(this->roughness);
    OrthonormalBasis sampleSpace(sampleDir);
    sampleDir += sampleSpace.getU() * offset.x();
    sampleDir += sampleSpace.getV() * offset.y();

    Point hitpoint = hit.getHitpoint();

    Ray ray(hitpoint+(sampleDir*.001), sampleDir);
    auto result = scene.traceRay(ray);

    const AreaLight* lightHit = nullptr;
    double bestT = result->t;
    for(const auto& light : scene.getAreaLights()){
        auto lightIntersect = Triangle::intersect(ray, light->a, light->b, light->c);
        if(lightIntersect.has_value() && bestT > lightIntersect->t){
            lightHit = &*light;
            bestT = lightIntersect->t;
        }
    }

    //TODO: non-zero roughness should include point lights

    if(lightHit != nullptr){
        return lightHit->color * lightHit->intensity;
    }

    if(result.has_value()){
        auto hitColor = result->getModelNode().getData().getMaterial().getColorFor(*result, scene, depth + 1);
        return hitColor;
    }

    return RGB::BLACK;
}




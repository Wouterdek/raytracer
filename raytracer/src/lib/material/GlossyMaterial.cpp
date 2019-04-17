#include "GlossyMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "math/OrthonormalBasis.h"
#include "math/Constants.h"
#include "NormalMapSampler.h"
#include <random>
#include <cmath>

namespace {
    thread_local std::random_device randDev;
    std::uniform_real_distribution<float> randAngle(0,2*PI);
    std::uniform_real_distribution<float> randUniform(0,1);
};

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

    float angle = randAngle(randDev);
    float dist = randUniform(randDev) * this->roughness;
    float xOffset = std::cos(angle)*dist;
    float yOffset = std::sin(angle)*dist;
    OrthonormalBasis sampleSpace(sampleDir);
    sampleDir += sampleSpace.getU() * xOffset;
    sampleDir += sampleSpace.getV() * yOffset;

    Point hitpoint = hit.getHitpoint();


    auto result = scene.traceRay(Ray(hitpoint+(sampleDir*.001), sampleDir));

    if(result.has_value()){
        auto hitColor = result->getModelNode().getData().getMaterial().getColorFor(*result, scene, depth + 1);
        return hitColor;
    }

    return RGB::BLACK;
}




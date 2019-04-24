#include "math/Constants.h"
#include "GlassMaterial.h"
#include "scene/renderable/Scene.h"
#include "scene/renderable/SceneRayHitInfo.h"

RGB GlassMaterial::getColorFor(const SceneRayHitInfo &hit, const Scene &scene, int depth) const
{
    if(depth > 5){
        return RGB::BLACK;
    }

    auto normal = hit.normal;
    auto hitpoint = hit.getHitpoint();

    Vector3 incomingDir = -hit.ray.getDirection();
    Vector3 reflectionRayDir = incomingDir + ((2.0*normal.dot(incomingDir))*normal);
    Ray reflectionRay(hitpoint + reflectionRayDir * 0.001, reflectionRayDir);

    auto reflectionHit = scene.traceRay(reflectionRay);
    RGB reflection{};
    if(reflectionHit.has_value()){
        reflection = reflectionHit->getModelNode().getData().getMaterial().getColorFor(*reflectionHit, scene, depth+1);
    }

    //double woDotN = incomingDir.dot(normal);
    double woDotN = normal.dot(incomingDir);
    auto incomingAngle = woDotN; //assuming wo and n are normalized
    bool isInternalRay = woDotN < 0;
    double cosTransmission = sqrt(1.0-((1.0-pow(incomingAngle, 2.0))/pow(ior, 2.0)));
    //auto transmissionAngle = acos(cosTransmission);

    //if(isInternalRay && transmissionAngle > (PI/2.0))
    if(isInternalRay && 1-((1-pow(incomingAngle, 2.0))/pow(ior, 2.0)) < 0)
    {
        return reflection;
    }

    /*Vector3 i = normal.cross(normal.cross(incomingDir));
        Vector3 j = normal;

        Vector3 transmittedRayDir = (i * cos(-PI + transmissionAngle)) + (j * sin(-PI + transmissionAngle));*/
    Vector3 transmittedRayDir = (incomingDir/ior) - (cosTransmission - incomingAngle/ior)*normal;
    Ray transmittedRay(hitpoint + transmittedRayDir * 0.001, transmittedRayDir);

    auto transmissionHit = scene.traceRay(transmittedRay);
    RGB transmission = RGB::BLACK;
    if(transmissionHit.has_value()){
        transmission = transmissionHit->getModelNode().getData().getMaterial().getColorFor(*transmissionHit, scene, depth+1); //todo depth
    }
    return transmission + reflection;
}

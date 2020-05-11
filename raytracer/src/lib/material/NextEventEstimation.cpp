#include "NextEventEstimation.h"

#include "math/FastRandom.h"
#include "math/Sampler.h"
#include "math/Constants.h"

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
        auto geometricFactor = 1.0f / (4.0f * PI * (lampT * lampT)); // unit: 1/m^2

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
        auto geometricFactor = lampAngle / (lampT * lampT);

        auto irradianceFromLamp = lightRadiance * geometricFactor;
        return irradianceFromLamp * light.getSurfaceArea();
    }
    return RGB::BLACK;
}

RGB neeDirectionalLight(const DirectionalLight& light, const Scene& scene, const Point& hitpoint, const Vector3& normal, int sampleI, int sampleCount, /* OUT */ Vector3& lightDirection)
{
    lightDirection = sampleUniformSteradianSphere(-light.direction, light.angle);

    Ray visibilityRay(hitpoint + (lightDirection * 0.0001f), lightDirection);
    bool isVisible = !scene.testVisibility(visibilityRay, INFINITY).has_value();
    if (isVisible)
    {
        auto irradianceFromLamp = light.color * light.intensity;
        return irradianceFromLamp;
    }
    return RGB::BLACK;
}

RGB NextEventEstimation::sample(const Scene& scene, const Point& hitpoint, const Vector3& normal, int sampleI, int sampleCount, /* OUT */ Vector3& lightDirection)
{
    bool hasPointLights = !scene.getPointLights().empty();
    bool hasAreaLights = !scene.getAreaLights().empty();
    bool hasDirectionalLights = !scene.getDirectionalLights().empty();

    /*
     * To choose a light type, uniformly sample a number in [0;1] and pick corresponding type from diagram below
     *
     * ----------------
     * | PL | AL | DL |
     * ----------------
     * 0              1
     *
     * Types are removed from the diagram if they are not present in the scene and the space is divided equally amongst all present types.
     * After sampling, the resulting light intensity is divided by the probability of choosing the light type, as should be done when sampling a sum
     */
    float totalAvailableLightTypes = (hasAreaLights ? 1.0f : 0.0f) + (hasPointLights ? 1.0f : 0.0f) + (hasDirectionalLights ? 1.0f : 0.0f);
    float pointLightProbability = (hasPointLights ? 1.0f : 0.0f) / totalAvailableLightTypes;
    float areaLightProbability = (hasAreaLights ? 1.0f : 0.0f) / totalAvailableLightTypes;
    float directionalLightProbability = (hasDirectionalLights ? 1.0f : 0.0f) / totalAvailableLightTypes;

    float choice = Rand::unit();
    if(choice < pointLightProbability) // Choose point light
    {
        float chosenLightProbability = 1.0f/scene.getPointLights().size();
        auto lightIndex = Rand::intInRange(scene.getPointLights().size()-1);
        const auto& light = *scene.getPointLights()[lightIndex].get();

        return neePointLight(light, scene, hitpoint, normal, lightDirection).divide(pointLightProbability * chosenLightProbability);
    }
    else if(choice < pointLightProbability + areaLightProbability) // Choose area light
    {
        float chosenLightProbability = 1.0f/scene.getAreaLights().size();
        auto lightIndex = Rand::intInRange(scene.getAreaLights().size()-1);
        const auto& light = *scene.getAreaLights()[lightIndex].get();

        return neeAreaLight(light, scene, hitpoint, normal, sampleI, sampleCount, lightDirection).divide(areaLightProbability * chosenLightProbability);
    }
    else if(hasDirectionalLights)
    {
        float chosenLightProbability = 1.0f/scene.getDirectionalLights().size();
        auto lightIndex = Rand::intInRange(scene.getDirectionalLights().size()-1);
        const auto& light = *scene.getDirectionalLights()[lightIndex].get();

        return neeDirectionalLight(light, scene, hitpoint, normal, sampleI, sampleCount, lightDirection).divide(directionalLightProbability * chosenLightProbability);
    }
    else
    {
        return RGB::BLACK;
    }
}
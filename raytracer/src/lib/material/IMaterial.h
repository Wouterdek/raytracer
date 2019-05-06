#pragma once
#include "film/RGB.h"
#include "math/RayHitInfo.h"

class SceneRayHitInfo;
class Scene;

/*
 * Interface to represent materials (BxDFs).
 */
class IMaterial
{
public:
	virtual ~IMaterial() = default;

	/**
	 * Calculate total radiance transmitted towards an observer.
	 * @param hit specifies the model, point on the shape and direction of the observer.
	 * @param scene the scene in which the model is present.
	 * @param depth the recursion depth, which is used to stop calculations when the contributions become insignificant.
	 * @return the total transmitted radiance.
	 */
	virtual RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const = 0;

	/**
	 * Take an incoming radiance "particle" (photon), choose an outgoing (bounced, transmitted, ...) direction
	 * using importance sampling and calculate the remaining radiance of the outgoing photon.
	 *
	 * The outgoing direction is chosen using importance sampling, i.e. it is more likely to have a high contribution
	 * to the exitant irradiance for the specified hitpoint.
	 * @param hit the direction and position of the incoming photon.
	 * @param incomingEnergy energy of the incoming photon.
	 * @return The outgoing photon ray, the remaining energy in that photon and the diffuseness of the transport.
	 */
    virtual std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo& hit, const RGB& incomingEnergy) const;
};

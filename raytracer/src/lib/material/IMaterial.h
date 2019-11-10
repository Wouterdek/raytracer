#pragma once

#include "scene/renderable/SceneRayHitInfo.h"
#include "film/RGB.h"
#include "utility/DynAllocTree.h"
#include <functional>
#include <optional>
#include <vector>

class Scene;

enum class TransportType : unsigned char
{
    bounce, transmit
};

class TransportNode
{
public:
    SceneRayHitInfo hit;
    float pathTerminationChance;
    Vector3 transportDirection;
    float specularity;
    TransportType type;
    bool isEmissive;
    DynAllocTree<48, 3, 2, unsigned char> metadata;

    explicit TransportNode(SceneRayHitInfo hit) : hit(std::move(hit)), pathTerminationChance(), transportDirection(), type(), specularity(), isEmissive(), metadata()
        {}
};

struct TransportContext
{
    const Scene& scene;
    std::vector<TransportNode>& path;
    int curI = 0;
    int sampleI = 0;
    int sampleCount = 0;

    TransportContext(const Scene &scene, std::vector<TransportNode>& path) : scene(scene), path(path)
    {}

    TransportNode& getCurNode()
    {
        return path[curI];
    }
};

struct TransportBuildContext : TransportContext
{
    std::optional<SceneRayHitInfo> nextHit;
    std::optional<std::function<void()>> nextNodeCallback;

    TransportBuildContext(const Scene &scene, std::vector<TransportNode>& path) : TransportContext(scene, path)
    {}
};

/*
 * Interface to represent materials (BxDFs).
 */
class IMaterial
{
public:
	virtual ~IMaterial() = default;

	virtual void sampleTransport(TransportBuildContext &ctx) const = 0;
	virtual RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const = 0;


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

    virtual bool hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const;
};

#pragma once

#include "IMaterial.h"
#include <vector>
#include <memory>

class CompositeMaterial : public IMaterial {
public:
    CompositeMaterial();

    void sampleTransport(TransportBuildContext &ctx) const override;
    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    std::tuple<Vector3, RGB, float> interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const override;
    bool hasVariance(const std::vector<TransportNode> &path, int curI, const Scene &scene) const override;

    void addMaterial(size_t firstTriangleI, size_t length, std::shared_ptr<IMaterial> material);

private:
    std::vector<std::pair<uint32_t, std::shared_ptr<IMaterial>>> materialMapping;

    uint32_t findListIndex(uint32_t triangleIndex) const;
    IMaterial* findMaterial(uint32_t triangleIndex) const;
    const IMaterial* findMaterial(const SceneRayHitInfo& hit) const;
};

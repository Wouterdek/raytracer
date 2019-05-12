#include <shape/TriangleMesh.h>
#include "CompositeMaterial.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include <stdint.h>

CompositeMaterial::CompositeMaterial() : materialMapping() {
    materialMapping.emplace_back(0, nullptr);
}

uint32_t CompositeMaterial::findListIndex(uint32_t searchIdx) const {
    size_t lowI = 0;
    size_t highI = materialMapping.size()-1;
    while(highI - lowI > 1){
        auto curI = lowI + ((highI-lowI) / 2);
        auto curStartTriangle = materialMapping[curI].first;
        if(searchIdx < curStartTriangle){
            highI = curI-1;
        }else{
            lowI = curI;
        }
    }

    size_t matI = lowI;
    if(highI - lowI == 1 && searchIdx >= materialMapping[highI].first){
        matI = highI;
    }

    return matI;
}

IMaterial* CompositeMaterial::findMaterial(uint32_t triangleIndex) const {
    auto matI = findListIndex(triangleIndex);
    return materialMapping[matI].second.get();
}

void CompositeMaterial::addMaterial(size_t firstTriangleI, size_t length, std::shared_ptr<IMaterial> material) {
    auto lastDummyI = materialMapping.back().first;
    if(firstTriangleI < lastDummyI){
        auto rangeEntryIdx = findListIndex(firstTriangleI);
        auto& rangeStartEntry = materialMapping[rangeEntryIdx];
        auto& rangeEndEntry = materialMapping[rangeEntryIdx+1];

        // Check if range is free
        if(rangeStartEntry.second != nullptr || firstTriangleI + (length-1) >= rangeEndEntry.first){ //assuming no 2 dummies are neighbours
            throw std::runtime_error("The triangle range already has an assigned material");
        }

        // Insert new material mapping entry (or replace dummy with real entry if idx equal)
        size_t newEntryIdx;
        if(rangeStartEntry.first == firstTriangleI){
            rangeStartEntry.second = material;
            newEntryIdx = rangeEntryIdx;
        }else{
            materialMapping.insert(materialMapping.begin() + rangeEntryIdx+1, std::make_pair(firstTriangleI, material));
            newEntryIdx = rangeEntryIdx+1;
        }

        // Insert new dummy to mark end of mapping entry (or don't if the range ends where the next entry starts)
        if(firstTriangleI+length != rangeEndEntry.first){
            materialMapping.insert(materialMapping.begin() + newEntryIdx+1, std::make_pair(firstTriangleI+length, nullptr));
        }
    }else if(firstTriangleI == lastDummyI){
        materialMapping.back().first = firstTriangleI;
        materialMapping.back().second = material;
        materialMapping.emplace_back(firstTriangleI + length, nullptr);
    }else{
        materialMapping.emplace_back(firstTriangleI, material);
        materialMapping.emplace_back(firstTriangleI + length, nullptr);
    }
}

const IMaterial* CompositeMaterial::findMaterial(const SceneRayHitInfo& hit) const
{
    auto triangleIdx = hit.triangleIndex;
    if(triangleIdx == UINT32_MAX)
    {
        return nullptr;
    }

    const auto& triangleMesh = dynamic_cast<const TriangleMesh&>(hit.getModelNode().getData().getShape());
    if(triangleMesh.getData().permutation.has_value()){
        triangleIdx = triangleMesh.getData().permutation->at(triangleIdx);
    }

    auto* material = this->findMaterial(triangleIdx);
    assert(material != nullptr);

    return material;
}

RGB CompositeMaterial::getTotalRadianceTowards(const SceneRayHitInfo& hit, const Scene& scene, int depth) const
{
    const IMaterial* mat = findMaterial(hit);
    if(mat == nullptr)
    {
        return RGB::BLACK;
    }

    return mat->getTotalRadianceTowards(hit, scene, depth);
}

std::tuple<Vector3, RGB, float> CompositeMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    const IMaterial* mat = findMaterial(hit);
    assert(mat != nullptr);

    return mat->interactPhoton(hit, incomingEnergy);
}

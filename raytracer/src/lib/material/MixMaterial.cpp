//
// Created by wouter on 17/04/19.
//

#include "MixMaterial.h"

RGB MixMaterial::getColorFor(const SceneRayHitInfo &hit, const Scene &scene, int depth) const {
    RGB contribA;
    if(mixFactor < 1){
        contribA = this->first->getColorFor(hit, scene, depth);
    }

    RGB contribB;
    if(mixFactor > 0){
        contribB = this->second->getColorFor(hit, scene, depth);
    }

    return (contribA * (1.0-mixFactor)) + (contribB * mixFactor);
}

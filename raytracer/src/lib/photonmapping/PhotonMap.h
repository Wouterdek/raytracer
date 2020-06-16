#pragma once
#include "Photon.h"
#include "KDTree.h"

using PhotonMap = KDTree<Photon, &Photon::getPosition>;
using PhotonRayMap = pluckertree::Tree<PhotonRay, &PhotonRay::line>;

enum class PhotonMapMode
{
    none,
    caustics,
    full,
    rays
};
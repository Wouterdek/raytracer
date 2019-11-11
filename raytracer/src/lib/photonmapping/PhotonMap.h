#pragma once
#include "Photon.h"
#include "KDTree.h"

using PhotonMap = KDTree<Photon, &Photon::getPosition>;

enum class PhotonMapMode
{
    none,
    caustics,
    full
};
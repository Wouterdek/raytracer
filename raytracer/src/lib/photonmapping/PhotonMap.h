#pragma once
#include "Photon.h"
#include "KDTree.h"

using PhotonMap = KDTree<Photon, &Photon::getPosition>;
using PhotonRayMap = PhotonRayList;

enum class PhotonMapMode
{
    none,
    caustics,
    full,
    rays
};
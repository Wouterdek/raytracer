#pragma once
#include "Photon.h"
#include "KDTree.h"

using PhotonMap = KDTree<Photon, &Photon::getPosition>;
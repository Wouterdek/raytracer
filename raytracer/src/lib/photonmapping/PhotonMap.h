#pragma once
#include "Photon.h"
#include "KDTree.h"
#include <tbb/tbb.h>

using PhotonMap = KDTree<Photon, &Photon::getPosition>;
//using PhotonMap = tbb::concurrent_vector<Photon>;
#include "array_offset_pointer.h"
#include "photonmapping/Photon.h"
#include "photonmapping/KDTree.h"

template<> KDTreeNode<Photon, &Photon::getPosition, array_offset_pointer>* array_offset_pointer<KDTreeNode<Photon, &Photon::getPosition, array_offset_pointer>>::array = nullptr;
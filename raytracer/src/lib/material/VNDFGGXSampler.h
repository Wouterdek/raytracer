#pragma once

#include "math/Vector3.h"

// Samples the visible normal distribution function for GGX distributions
// Used for finding normals of a rough surface in a microfacet model
// See: Importance Sampling Microfacet-Based BSDFs using the Distribution of Visible Normals (Eric Heitz 1, Eugene d'Eon)
class VNDFGGXSampler
{
public:
    static Vector3 sample(const Vector3& smoothNormal, const Vector3& incoming, float roughness);
};

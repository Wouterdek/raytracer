#pragma once

#include "IMaterial.h"

class FlatMaterial : public IMaterial
{
public:
    void sampleTransport(TransportBuildContext &ctx) const override;
};
#include "FlatMaterial.h"

void FlatMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.pathTerminationChance = 1.0;
    transport.isEmissive = true;
}
#include "FresnelMixMaterial.h"

float FresnelMixMaterial::calcMixFactor(const SceneRayHitInfo &hit) const
{
    auto& normal = hit.normal;
    auto& incomingDir = hit.ray.getDirection();
    auto objToIncoming = -incomingDir;

    // Check for total internal reflection
    double nDotWo = normal.dot(objToIncoming);
    auto incomingAngle = nDotWo; //assuming wo and n are normalized //Careful: this is cos of angle between normal and Wo, which could be >90 (if internal ray)
    bool isInternalRay = nDotWo < 0;
    auto curRelIor = IOR / 1.0; //Assuming 1.0 for air
    if(isInternalRay)
    {
        curRelIor = 1.0 / curRelIor;
    }
    double cosTransmissionSqr = 1.0 - ((1.0 - (incomingAngle * incomingAngle)) / (curRelIor * curRelIor)); //incomingAngle could actually be -incomingAngle, but this doesn't matter as we square it anyway
    bool tir = cosTransmissionSqr < 0; // total internal reflection

    if(tir)
    {
        return 1.0;
    }
    else
    {
        if(isInternalRay)
        {
            nDotWo = -nDotWo;
        }
        double cosRefractAngle = sqrt(cosTransmissionSqr);

        auto rs = ((curRelIor * nDotWo) - cosRefractAngle) / ((curRelIor * nDotWo) + cosRefractAngle);
        auto rp = (nDotWo - (curRelIor * cosRefractAngle)) / (nDotWo + (curRelIor * cosRefractAngle));
        auto kr = ((rs * rs) + (rp * rp)) / 2.0;
        auto reflectionWeight = kr;
        auto transmissionWeight = 1.0 - kr;
        auto totalWeight = reflectionWeight + transmissionWeight;
        return (float)(reflectionWeight / totalWeight);
    }
}

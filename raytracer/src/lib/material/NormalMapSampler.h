#pragma once

#include <math/RayHitInfo.h>
#include "math/Vector3.h"
#include "Texture.h"

inline Vector3 sample_normal_map(const RayHitInfo& hit, const TextureUInt8& normalMap)
{
    float x = fmodf(hit.texCoord.x(), 1.0f);
    float y = fmodf(hit.texCoord.y(), 1.0f);
    if(x < 0){ x = 1.0f + x;}
    if(y < 0){ y = 1.0f + y;}

    auto normalColor = normalMap.get(x * normalMap.getWidth(), y * normalMap.getHeight());
    auto normalInTangentSpace = Vector3(2.0f*(normalColor.getRed() - 0.5f), 2.0f*(normalColor.getGreen() - 0.5f), 2.0f*(normalColor.getBlue() - 0.5f));

    auto transform = Transformation::tangentToObject(hit.tangent, hit.normal);
    auto normal = transform.transform(normalInTangentSpace);
    normal.normalize();
    return normal;
}
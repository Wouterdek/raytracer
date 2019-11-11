#pragma once

#include <math/RayHitInfo.h>
#include "math/Vector3.h"
#include "Texture.h"

inline Vector3 sample_normal_map(const RayHitInfo& hit, const TextureUInt8& normalMap)
{
    float x = abs(fmod(hit.texCoord.x(), 1.0f));
    float y = abs(fmod(hit.texCoord.y(), 1.0f));
    auto normalColor = normalMap.get(x * normalMap.getWidth(), y * normalMap.getHeight());
    return Vector3(normalColor.getRed(), normalColor.getGreen(), normalColor.getBlue());
}
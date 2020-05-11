#pragma once

#include "film/RGB.h"
#include "math/Vector3.h"
#include "scene/renderable/Scene.h"

class NextEventEstimation {
public:
    NextEventEstimation() = delete;

    static RGB sample(
            const Scene& scene, const Point& hitpoint, const Vector3& normal, int sampleI, int sampleCount,
            /* OUT */ Vector3& lightDirection);
};

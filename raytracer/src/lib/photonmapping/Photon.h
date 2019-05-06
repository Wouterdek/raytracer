#pragma once

#include <film/RGB.h>

struct Photon
{
    Point pos;
    Vector3 incomingDir;
    RGB energy;
    bool isCaustic;

    Photon(Point pos, Vector3 incomingDir, RGB energy, bool isCaustic)
            : pos(std::move(pos)), incomingDir(std::move(incomingDir)), energy(energy), isCaustic(isCaustic)
    {}

    const Point& getPosition() const
    {
        return pos;
    }
};

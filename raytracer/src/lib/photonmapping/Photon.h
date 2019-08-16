#pragma once

#include <film/RGB.h>

struct Photon
{
    Point pos;
    Vector3 incomingDir;
    Vector3 surfaceNormal;
    RGB energy;
    bool isCaustic;

    Photon(Point pos, Vector3 incomingDir, Vector3 surfaceNormal, RGB energy, bool isCaustic)
            : pos(std::move(pos)), incomingDir(std::move(incomingDir)), surfaceNormal(std::move(surfaceNormal)), energy(energy), isCaustic(isCaustic)
               {}

    Photon() : pos(), incomingDir(), surfaceNormal(), energy(), isCaustic(false)
        {}

    const Point& getPosition() const
    {
        return pos;
    }
};

#pragma once

#include <film/RGB.h>
#include <math/Vector3.h>
#include <vector>

struct Photon
{
    Point pos;
    Vector3 surfaceNormal;
    RGB energy;
    bool isCaustic;

    Photon(Point pos, Vector3 incomingDir, Vector3 surfaceNormal, RGB energy, bool isCaustic)
            : pos(std::move(pos)), surfaceNormal(std::move(surfaceNormal)), energy(std::move(energy)), isCaustic(isCaustic)
               {}

    Photon() : pos(), surfaceNormal(), energy(), isCaustic(false)
        {}

    const Point& getPosition() const
    {
        return pos;
    }
};

struct PhotonRay
{
    Vector3 direction;
    Vector3 moment;
    RGB energy;

    PhotonRay(Vector3 direction, Vector3 moment, RGB energy)
            : direction(std::move(direction)), moment(std::move(moment)), energy(std::move(energy))
    {}

    PhotonRay() : direction(), moment(), energy()
    {}
};

#ifdef NO_TBB
using PhotonList = std::vector<Photon>;
using PhotonRayList = std::vector<PhotonRay>;
#else
#include <tbb/tbb.h>
using PhotonList = tbb::concurrent_vector<Photon>;
using PhotonRayList = tbb::concurrent_vector<PhotonRay>;
#endif

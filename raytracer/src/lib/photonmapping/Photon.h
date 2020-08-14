#pragma once

#include <film/RGB.h>
#include <math/Vector3.h>
#include <vector>
#include <Pluckertree.h>

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
    pluckertree::Line line;
    float originT;
    RGB energy;

    PhotonRay(Vector3 direction, Vector3 moment, float originT, RGB energy)
            : line(std::move(direction), std::move(moment)), originT(originT), energy(std::move(energy))
    {}

    PhotonRay() : line(Vector3(), Vector3()), originT(), energy()
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

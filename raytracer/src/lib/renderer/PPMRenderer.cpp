#include <math/Constants.h>
#include "PPMRenderer.h"
#include "photonmapping/PhotonTracer.h"
#include "photonmapping/KDTreeBuilder.h"

struct HitpointInfo
{
    SceneRayHitInfo rayhit;
    RGB weight;
    float radius = -1;
    int photonCount;
    RGB reflectedFlux;

    HitpointInfo(SceneRayHitInfo rayHitInfo)
        : rayhit(std::move(rayHitInfo))
    {}
};

void PPMRenderer::render(const Scene& scene, FrameBuffer& buffer, const Tile& tile, const RenderSettings& renderSettings, ProgressMonitor progressMon, bool multithreaded)
{
    std::vector<std::optional<HitpointInfo>> hits;
    hits.reserve(tile.getWidth() * tile.getHeight());

    const ICamera& camera = findCamera(scene);

    for (int y = tile.getYStart(); y < tile.getYEnd(); ++y)
    {
        for (int x = tile.getXStart(); x < tile.getXEnd(); ++x)
        {
            auto sample = Vector2(x + 0.5, y+0.5);
            auto ray = camera.generateRay(sample, buffer.getHorizontalResolution(), buffer.getVerticalResolution());
            auto hit = scene.traceRay(ray);
            if(hit.has_value())
            {
                hits[(y*tile.getWidth()) + x] = HitpointInfo(*hit);
            }
        }
    }

    tbb::concurrent_vector<Photon> photons;
    ProgressTracker progress(progressMon);

    PhotonTracer photonTracer;
    photonTracer.batchSize = 1000;
    photonTracer.photonsPerAreaLight = 1E2;
    photonTracer.photonsPerPointLight = 1E2;
    int iterations = 500;

    auto tiles = subdivideTilePerCores(tile);

    for(int i = 0; i < iterations; i++)
    {
        photons.clear();
        tbb::task_list taskList;
        PhotonTracer::size_type taskCount;
        photonTracer.createPhotonTracingTasks(scene, taskList, taskCount, photons, progress);
        tbb::task::spawn_root_and_wait(taskList);

        auto tree = KDTreeBuilder::build(photons);

        tbb::parallel_for_each(tiles.begin(), tiles.end(), [&masterTile = tile, &scene, &buffer, &renderSettings, &progress, &camera, &hits, &tree](const Tile& tile)
        {
            std::vector<const Photon*> neighbourPhotons;
            for (int y = tile.getYStart(); y < tile.getYEnd(); ++y)
            {
                for (int x = tile.getXStart(); x < tile.getXEnd(); ++x)
                {
                    auto& hitinfo = hits[(y*masterTile.getWidth()) + x];
                    if(!hitinfo.has_value())
                    {
                        continue;
                    }

                    auto hitpoint = hitinfo->rayhit.getHitpoint();

                    neighbourPhotons.clear();
                    if(hitinfo->radius < 0)
                    {
                        neighbourPhotons.resize(10);
                        hitinfo->radius = tree.getElementsNearestTo(hitpoint, neighbourPhotons.size(), neighbourPhotons);
                        hitinfo->photonCount = neighbourPhotons.size();
                        //hitinfo->reflectedFlux
                        //hitinfo->weight
                        continue;
                    }

                    //Temp
                    //neighbourPhotons.resize(10);
                    //hitinfo->radius = tree.getElementsNearestTo(hitpoint, neighbourPhotons.size(), neighbourPhotons);

                    auto filter = [&dir = hitinfo->rayhit.normal](const Photon& photon)
                    {
                        return dir.dot(-photon.incomingDir) >= 0;
                    };
                    tree.getElementsInRadiusFrom(hitpoint, hitinfo->radius, filter, neighbourPhotons);
                    //auto newDensity = (neighbourPhotons.size() + hitinfo->photonCount) / (PI * pow(hitinfo->radius, 2.0f));
                    float alfa = 0.8f;
                    auto n = hitinfo->photonCount;
                    auto m = neighbourPhotons.size();
                    auto newRadius = hitinfo->radius * sqrt((n + alfa*m)/(n + m));

                    RGB totalEnergy;
                    for(const Photon* photon : neighbourPhotons)
                    {
                        totalEnergy += photon->energy.scale(std::max(0.0f, hitinfo->rayhit.normal.dot(-photon->incomingDir)));
                    }
                    auto ratio = pow(newRadius, 2.0) / pow(hitinfo->radius, 2.0);
                    hitinfo->reflectedFlux = (hitinfo->reflectedFlux + totalEnergy) * ratio;
                    hitinfo->radius = newRadius;

                    //buffer.setPixel(x, y, hitinfo->reflectedFlux);
                    //buffer.setPixel(x, y, RGB(hitinfo->radius));
                    buffer.setPixel(x, y, totalEnergy);
                }
            }
        });
    }
}

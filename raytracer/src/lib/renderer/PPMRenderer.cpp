#include "PPMRenderer.h"
#include "photonmapping/PhotonTracer.h"
#include "photonmapping/KDTreeBuilder.h"

void PPMRenderer::render(const Scene& scene, FrameBuffer& buffer, const Tile& tile, const RenderSettings& renderSettings, ProgressMonitor progressMon, bool multithreaded)
{
    std::vector<std::optional<SceneRayHitInfo>> hits;
    hits.reserve(tile.getWidth() * tile.getHeight());

    const ICamera& camera = findCamera(scene);

    for (int y = tile.getYStart(); y < tile.getYEnd(); ++y)
    {
        for (int x = tile.getXStart(); x < tile.getXEnd(); ++x)
        {
            auto sample = Vector2(x + 0.5, y+0.5);
            auto ray = camera.generateRay(sample, buffer.getHorizontalResolution(), buffer.getVerticalResolution());
            hits[(y*tile.getWidth()) + x] = scene.traceRay(ray);
        }
    }

    tbb::concurrent_vector<Photon> photons;
    ProgressTracker progress(progressMon);

    PhotonTracer photonTracer;
    photonTracer.batchSize = 1000;
    photonTracer.photonsPerAreaLight = 1E5;
    photonTracer.photonsPerPointLight = 1E5;
    int iterations = 50;

    auto tiles = subdivideTilePerCores(tile);

    for(int i = 0; i < iterations; i++)
    {
        tbb::task_list taskList;
        PhotonTracer::size_type taskCount;
        photonTracer.createPhotonTracingTasks(scene, taskList, taskCount, photons, progress);
        tbb::task::spawn_root_and_wait(taskList);

        auto tree = KDTreeBuilder::build(photons);

        tbb::parallel_for_each(tiles.begin(), tiles.end(), [&masterTile = tile, &scene, &buffer, &renderSettings, &progress, &camera, &hits, &tree](const Tile& tile)
        {
            for (int y = tile.getYStart(); y < tile.getYEnd(); ++y)
            {
                for (int x = tile.getXStart(); x < tile.getXEnd(); ++x)
                {
                    auto hit = hits[(y*masterTile.getWidth()) + x];
                    if(!hit.has_value())
                    {
                        continue;
                    }

                    auto hitpoint = hit->getHitpoint();
                    std::vector<const Photon*> neighbourPhotons(10);
                    tree.getElementsNearestTo(hitpoint, neighbourPhotons.size(), neighbourPhotons);

                    //RGB value {};
                    float maxDistSqr = 0;
                    for(const Photon* photon : neighbourPhotons)
                    {
                        if(hit->ray.getDirection().dot(-photon->incomingDir) < 0)
                        {
                            continue;
                        }

                        auto squaredDist = (photon->getPosition() - hitpoint).squaredNorm();
                        maxDistSqr = std::max(maxDistSqr, squaredDist);
                        /*auto val = 1.0/(sqrt(squaredDist) + 1);
                        value += RGB(photon->isCaustic ? 0: val, photon->isCaustic ? val : 0, 0);*/
                        //value += brdf(photon->energy * 200.0, normal, -photon->incomingDir);
                    }
                    buffer.setPixel(x, y, RGB(maxDistSqr));
                }
            }
        });

        photons.clear();
    }
}

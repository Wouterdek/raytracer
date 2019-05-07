#include "Renderer.h"
#include <thread>
#include <random>
//#include <execution>
#include "tbb/tbb.h"
#include "math/Ray.h"
#include "camera/ICamera.h"
#include "utility/ProgressMonitor.h"
#include "math/UniformSampler.h"

#undef min

void Renderer::render(const Scene &scene, FrameBuffer &buffer, const Tile &tile, const RenderSettings &renderSettings, ProgressMonitor progressMon, bool multithreaded)
{
    std::vector<Tile> tiles;

    if(multithreaded)
    {
        tiles = subdivideTilePerCores(tile);
    }
    else
    {
        tiles.push_back(tile);
    }

    ProgressTracker progress(progressMon);
    progress.startNewJob("Rendering tiles", tiles.size());

    const ICamera& camera = findCamera(scene);

    tbb::parallel_for_each(tiles.begin(), tiles.end(), [&scene, &buffer, &renderSettings, &progress, &camera](const Tile& curTile)
    {
        for (int y = curTile.getYStart(); y < curTile.getYEnd(); ++y) {
            for (int x = curTile.getXStart(); x < curTile.getXEnd(); ++x) {
                RGB pixelValue{};

                for(int i = 0; i < renderSettings.aaLevel; i++)
                {
                    // create a ray through the center of the pixel.
                    Vector2 sample = Vector2(x, y) + sampleStratifiedSquare(renderSettings.aaLevel, i);
                    Ray ray = camera.generateRay(sample, buffer.getHorizontalResolution(), buffer.getVerticalResolution());

                    // test the scene on intersections
                    //auto start = std::chrono::high_resolution_clock::now();

                    auto hit = scene.traceRay(ray);

                    //auto finish = std::chrono::high_resolution_clock::now();
                    //double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count()/1000.0;

                    //buffer.setPixel(x, y, RGB(duration));
                    //maxVal = std::max(maxVal, duration);

                    /*RGB bvhMarker(0, BVHDiag::Levels, 0);
                    pixelValue = pixelValue.add(bvhMarker);
                    BVHDiag::Levels = 0;*/

                    // add a color contribution to the pixel
                    if (hit.has_value())
                    {
                        /*auto hitpoint = hit->getGeometryInfo().getHitpoint();
                        auto depth = std::log(hitpoint.norm())/4.0f;
                        depth = std::clamp<float>(depth, 0, 1);
                        buffer.setPixel(x, y, RGB(depth));*/

                        pixelValue = pixelValue.add(
                                hit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*hit, scene, 0));
                    }
                }

                buffer.setPixel(x, y, pixelValue.divide(renderSettings.aaLevel));
            }

            //std::cout << y * 100 / curTile.getYEnd() << "% done\r";
        }
        progress.signalTaskFinished();
    });
}

void Renderer::render(const Scene &scene, FrameBuffer &buffer, const RenderSettings &renderSettings, ProgressMonitor progressMon)
{
    render(scene, buffer, Tile(0, 0, buffer.getHorizontalResolution(), buffer.getVerticalResolution()), renderSettings, std::move(progressMon), true);
}

const ICamera& Renderer::findCamera(const Scene& scene)
{
    const ICamera* mainCam = nullptr;
    for(const auto& camera : scene.getCameras()){
        if(camera.getData().isMainCamera){
            mainCam = &camera.getData();
        }
    }
    if(mainCam == nullptr){
        if(!scene.getCameras().empty()){
            mainCam = &scene.getCameras()[0].getData();
        }else{
            throw std::runtime_error("No camera in scene.");
        }
    }
    return *mainCam;
}

std::vector<Tile> Renderer::subdivideTilePerCores(const Tile &tile, int taskMultiplier)
{
    const int cpuCount = std::thread::hardware_concurrency();

    const auto taskCount = std::min({ cpuCount * taskMultiplier, tile.getWidth(), tile.getHeight() });

    const auto width = tile.getWidth() / taskCount;
    const auto height = tile.getHeight() / taskCount;

    return tile.subdivide(width, height);
}

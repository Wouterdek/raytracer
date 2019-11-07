#include "Renderer.h"
#include <thread>
#include "tbb/tbb.h"
#include "math/Ray.h"
#include "camera/ICamera.h"
#include "utility/ProgressMonitor.h"
#include "math/Sampler.h"
#include "math/FastRandom.h"

#undef min

// Returns true if the returned path ends prematurely
bool samplePath(std::vector<TransportNode>& path, int samplingStartIndex, int maxPathLength, const Scene& scene, int materialAALevel, int sampleI)
{
    TransportBuildContext ctx(scene, path);
    ctx.curI = samplingStartIndex;

    bool pathDone = false;
    bool pathTerminated = false; //Max length was reached or russian roulette triggered
    std::optional<SceneRayHitInfo> hit;
    std::optional<std::function<void()>> curNodeCallback {};
    if(samplingStartIndex > 0)
    {
        //TODO: should somehow retrieve callback of ctx.curI-1 here
    }
    bool hasPassedNodeWithVariance = false;
    do{
        auto& curNode = ctx.getCurNode();
        bool curNodeHasVariance = ctx.getCurNode().hit.getModelNode().getData().getMaterial().hasVariance(ctx.path, ctx.curI, scene);
        bool isFirstNodeWithVariance = curNodeHasVariance && !hasPassedNodeWithVariance;
        ctx.sampleCount = isFirstNodeWithVariance ? materialAALevel : 1;
        ctx.sampleI = isFirstNodeWithVariance ? sampleI : 0;
        hasPassedNodeWithVariance = hasPassedNodeWithVariance || curNodeHasVariance;

        curNode.hit.getModelNode().getData().getMaterial().sampleTransport(ctx);
        hit = ctx.nextHit;
        ctx.nextHit.reset();

        if(curNodeCallback.has_value())
        {
            (*curNodeCallback)();

            if(ctx.path[ctx.curI-1].isEmissive || ctx.path[ctx.curI-1].pathTerminationChance == 1.0) //bit of a hack here, I know
            {
                break;
            }
        }
        curNodeCallback = ctx.nextNodeCallback;
        ctx.nextNodeCallback.reset();

        pathTerminated = (ctx.curI+1 == maxPathLength) || (!curNode.isEmissive && Rand::unit() < curNode.pathTerminationChance);
        pathDone = curNode.isEmissive || pathTerminated;

        if(!pathDone)
        {
            if(!hit.has_value())
            {
                Ray ray(curNode.hit.getHitpoint() + (curNode.transportDirection * 0.0001f), curNode.transportDirection);
                hit = scene.traceRay(ray);
            }
            if (hit.has_value())
            {
                if(path.size() < (ctx.curI+2))
                {
                    path.emplace_back(*hit);
                }
                else
                {
                    path[ctx.curI+1] = TransportNode(*hit);
                }
            }
            else
            {
                pathDone = true;
            }
        }
        ctx.curI++;
        /*if(ctx.curI == maxPathLength)
        {
            pathDone = true;
        }*/
    }while(!pathDone);

    path.erase(path.begin()+ctx.curI, path.end());
    return pathTerminated;
}

RGB calculatePathEnergy(std::vector<TransportNode>& path, const Scene& scene)
{
    // Calculate light transported along this path
    const auto& lastNode = path[path.size()-1];

    RGB energy;
    if(scene.hasEnvironmentMaterial() && !lastNode.isEmissive)
    {
        energy = scene.getEnvironmentMaterial().getRadiance(scene, lastNode.transportDirection);
    }
    for(int pathI = path.size()-1; pathI >= 0; --pathI) //From path end to front
    {
        auto& curPathElem = path[pathI];
        energy = curPathElem.hit.getModelNode().getData().getMaterial().bsdf(scene, path, pathI, curPathElem, energy);
        if(!curPathElem.isEmissive)
        {
            energy = energy.divide(1.0f-curPathElem.pathTerminationChance);
        }
    }
    return energy;
}

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
        std::vector<TransportNode> path;
        const int maxPathLength = 10;
        path.reserve(maxPathLength);

        for (int y = curTile.getYStart(); y < curTile.getYEnd(); ++y) {
            for (int x = curTile.getXStart(); x < curTile.getXEnd(); ++x) {
                RGB pixelValue{};

                for(int i = 0; i < renderSettings.geometryAAModifier; i++)
                {
                    /**V3**/
                    // Build new path
                    path.clear();
                    Vector2 sample = Vector2(x, y) + sampleUniformStratifiedSquare(renderSettings.geometryAAModifier, i);
                    Ray ray = camera.generateRay(sample, buffer.getHorizontalResolution(), buffer.getVerticalResolution());
                    auto hit = scene.traceRay(ray);
                    bool pathWasTerminated;
                    if (hit.has_value())
                    {
                        auto& pathNode = path.emplace_back(*hit);
                        pathWasTerminated = samplePath(path, 0, maxPathLength, scene, renderSettings.materialAAModifier, 0);
                    }
                    else
                    {
                        continue;
                    }

                    int firstNodeWithVariance = 0;
                    for(; firstNodeWithVariance < path.size(); firstNodeWithVariance++)
                    {
                        if(path[firstNodeWithVariance].hit.getModelNode().getData().getMaterial().hasVariance(path, firstNodeWithVariance, scene))
                        {
                            break;
                        }
                    }

                    RGB matSample {};
                    if(!pathWasTerminated)
                    {
                        matSample = calculatePathEnergy(path, scene);
                    }
                    if(firstNodeWithVariance < path.size())
                    {
                        for(int j = 1; j < renderSettings.materialAAModifier; j++)
                        {
                            // From the first geometry hit on, resample the transport path if the bsdf at the hitpoint has variance.
                            pathWasTerminated = samplePath(path, firstNodeWithVariance, maxPathLength, scene, renderSettings.materialAAModifier, j);
                            if(!pathWasTerminated)
                            {
                                matSample = matSample.add(calculatePathEnergy(path, scene));
                            }
                        }
                        matSample = matSample.divide(renderSettings.materialAAModifier);
                    }
                    pixelValue = pixelValue.add(matSample);


                    /**V2**/
                    // create a ray through the pixel.
                    /*Vector2 sample = Vector2(x, y) + sampleStratifiedSquare(renderSettings.geometryAAModifier, i);
                    Ray ray = camera.generateRay(sample, buffer.getHorizontalResolution(), buffer.getVerticalResolution());
                    auto hit = scene.traceRay(ray);

                    if (hit.has_value())
                    {
                        const auto& material = hit->getModelNode().getData().getMaterial();
                        RGB curContribution = material.getTotalRadianceTowards(*hit, scene, 0);
                        int curMatSampleCount = material.hasVariance(*hit, scene) ? renderSettings.materialAAModifier : 1;

                        if(curMatSampleCount > 1)
                        {
                            auto materialSamplesToDo = renderSettings.materialAAModifier - 1;
                            for(int j = 0; j < materialSamplesToDo; j++)
                            {
                                curContribution = curContribution.add(material.getTotalRadianceTowards(*hit, scene, 0));
                            }
                        }
                        pixelValue = pixelValue.add(curContribution.divide(curMatSampleCount));
                    }*/

                    /**V1**/
                    // test the scene on intersections
                    //auto start = std::chrono::high_resolution_clock::now();



                    //auto finish = std::chrono::high_resolution_clock::now();
                    //double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count()/1000.0;

                    //buffer.setPixel(x, y, RGB(duration));
                    //maxVal = std::max(maxVal, duration);

                    /*RGB bvhMarker(0, BVHDiag::Levels, 0);
                    pixelValue = pixelValue.add(bvhMarker);
                    BVHDiag::Levels = 0;*/


                    // add a color contribution to the pixel
                    /*if (hit.has_value())
                    {*/
                        /*auto hitpoint = hit->getGeometryInfo().getHitpoint();
                        auto depth = std::log(hitpoint.norm())/4.0f;
                        depth = std::clamp<float>(depth, 0, 1);
                        buffer.setPixel(x, y, RGB(depth));*/

                        //pixelValue = pixelValue.add(hit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*hit, scene, 0));
                        //hit->getModelNode().getData().getMaterial().getTotalRadianceTowards(*hit, scene, 0);

                        /*RGB kdMarker(0, KDTreeDiag::Levels, 0);
                        pixelValue = pixelValue.add(kdMarker);
                        KDTreeDiag::Levels = 0;*/
                    //}
                }

                buffer.setPixel(x, y, pixelValue.divide(renderSettings.geometryAAModifier));
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

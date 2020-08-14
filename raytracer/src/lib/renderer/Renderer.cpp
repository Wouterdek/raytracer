#include "Renderer.h"
#include <thread>
#include <math/Triangle.h>
#include "math/Ray.h"
#include "camera/ICamera.h"
#include "utility/ProgressMonitor.h"
#include "math/Sampler.h"
#include "math/FastRandom.h"
#include "utility/Task.h"
#include "Pluckertree.h"

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

class RenderTileTask : public Task
{
private:
    const Tile& tile;
    const RenderSettings& renderSettings;
    const ICamera& camera;
    const Scene& scene;
    FrameBuffer& buffer;
    std::shared_ptr<FrameBuffer>& perfBuffer;
    ProgressTracker& progress;

public:
    RenderTileTask(const Tile &tile, const RenderSettings &renderSettings, const ICamera &camera, const Scene &scene, FrameBuffer &buffer, std::shared_ptr<FrameBuffer>& perfBuffer, ProgressTracker &progress)
                   : tile(tile), renderSettings(renderSettings), camera(camera), scene(scene), buffer(buffer), perfBuffer(perfBuffer), progress(progress)
                   {}

    void execute() override
    {
        std::vector<TransportNode> path;
        const int maxPathLength = 10;
        path.reserve(maxPathLength);

        std::vector<std::optional<SceneRayHitInfo>> hits(renderSettings.geometryAAModifier);

        for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
            for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
                RGB pixelValue{};
                RGB perfPixelValue{};
                unsigned int visited = 0;

                // Trace eye rays
                /*for(int i = 0; i < renderSettings.geometryAAModifier; i++) {
                    Ray ray = camera.generateRay(
                            Vector2(x, y), buffer.getHorizontalResolution(), buffer.getVerticalResolution(),
                            renderSettings.geometryAAModifier, i
                    );
                    hits[i] = scene.traceRay(ray);
                }*/
                {
                    auto rayBundles = renderSettings.geometryAAModifier / RayBundleSize;
                    auto nonbundledRays = renderSettings.geometryAAModifier % RayBundleSize;
                    for(int i = 0; i < rayBundles; i++) {
                        RayBundle rays;
                        for(RBSize_t rayI = 0; rayI < RayBundleSize; ++rayI)
                        {
                            rays[rayI] = camera.generateRay(
                                    Vector2(x, y), buffer.getHorizontalResolution(), buffer.getVerticalResolution(),
                                    renderSettings.geometryAAModifier, (i * RayBundleSize) + rayI
                            );
                        }
                        auto bundleHits = scene.traceRays(rays);
                        std::copy(bundleHits.begin(), bundleHits.end(), hits.begin() + (i * RayBundleSize));
                    }
                    for(int i = 0; i < nonbundledRays; i++) {
                        auto totalRayI = (rayBundles * RayBundleSize) + i;
                        Ray ray = camera.generateRay(
                                Vector2(x, y), buffer.getHorizontalResolution(), buffer.getVerticalResolution(),
                                renderSettings.geometryAAModifier, totalRayI
                        );
                        hits[totalRayI] = scene.traceRay(ray);
                    }
                }

                // Path tracing
                for(int i = 0; i < renderSettings.geometryAAModifier; i++)
                {
                    // Build new path
                    path.clear();

                    auto& hit = hits[i];
                    auto& ray = hits[i]->ray;

                    // Check if there is an area light that gives a closer hit
                    {
                        float bestT = hit.has_value() ? hit->t : INFINITY;
                        Triangle::TriangleIntersection lightHit;
                        AreaLight* light = nullptr;
                        for(const auto& areaLight : scene.getAreaLights())
                        {
                            if(Triangle::intersect(ray, areaLight->a, areaLight->b, areaLight->c, lightHit)
                               && (bestT > lightHit.t))
                            {
                                light = &*areaLight;
                                bestT = lightHit.t;
                            }
                        }
                        if(light != nullptr)
                        {
                            auto lightEnergy = light->color * light->intensity;
                            auto lightIrradiance = lightEnergy.divide(light->getSurfaceArea());
                            pixelValue = pixelValue.add(lightIrradiance.divide(2));
                            continue;
                        }
                    }

                    bool pathWasTerminated;
                    if (hit.has_value())
                    {
                        auto& pathNode = path.emplace_back(*hit);
                        pathWasTerminated = samplePath(path, 0, maxPathLength, scene, renderSettings.materialAAModifier, 0);

                        {//PERF
                            visited += pluckertree::Diag::visited;
                            pluckertree::Diag::visited = 0;
                            perfPixelValue = perfPixelValue.add(RGB(0, 0, KDTreeDiag::Levels));
                            KDTreeDiag::Levels = 0;
                        }
                    }
                    else
                    {
                        if(scene.hasEnvironmentMaterial())
                        {
                            RGB energy = scene.getEnvironmentMaterial().getRadiance(scene, ray.getDirection());
                            pixelValue = pixelValue.add(energy);
                        }
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

                            {//PERF
                                visited += pluckertree::Diag::visited;
                                pluckertree::Diag::visited = 0;
                                perfPixelValue = perfPixelValue.add(RGB(0, 0, KDTreeDiag::Levels));
                                KDTreeDiag::Levels = 0;
                            }

                            if(!pathWasTerminated)
                            {
                                matSample = matSample.add(calculatePathEnergy(path, scene));
                            }
                        }
                        matSample = matSample.divide(renderSettings.materialAAModifier);
                    }
                    pixelValue = pixelValue.add(matSample);
                }

                buffer.setPixel(x, y, pixelValue.divide(renderSettings.geometryAAModifier));
                float log_visited = visited == 0 ? 0 : (float)std::log((long double)visited);
                perfPixelValue = perfPixelValue.add(RGB(visited, log_visited, 0));
                perfBuffer->setPixel(x, y, perfPixelValue);
            }
        }
        progress.signalTaskFinished();
    }
};

void Renderer::render(const Scene &scene, FrameBuffer &buffer, std::shared_ptr<FrameBuffer>& perfBuffer, const Tile &tile, const RenderSettings &renderSettings, ProgressMonitor progressMon, bool multithreaded)
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

    std::vector<std::unique_ptr<Task>> tasks;
    for(const auto& curTile : tiles)
    {
        tasks.push_back(std::make_unique<RenderTileTask>(
            curTile, renderSettings, camera, scene, buffer, perfBuffer, progress
        ));
    }
    Task::runTasks(tasks);
}

void Renderer::render(const Scene &scene, FrameBuffer &buffer, std::shared_ptr<FrameBuffer>& perfBuffer, const RenderSettings &renderSettings, ProgressMonitor progressMon)
{
    render(scene, buffer, perfBuffer, Tile(0, 0, buffer.getHorizontalResolution(), buffer.getVerticalResolution()), renderSettings, std::move(progressMon), true);
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

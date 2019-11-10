#include <chrono>
#include <iostream>
#include <filesystem>
#include <exception>

#include "material/PositionMaterial.h"
#include "material/CompositeMaterial.h"
#include "material/DiffuseMaterial.h"
#include "material/GlossyMaterial.h"
#include "material/NormalMaterial.h"
#include "material/TexCoordMaterial.h"
#include "io/PNG.h"
#include "io/EXRWriter.h"
#include "io/GLTF.h"
#include "io/OBJLoader.h"
#include "io/TileFile.h"
#include "io/PPMFile.h"
#include "renderer/PPMRenderer.h"
#include "renderer/Renderer.h"
#include "film/FrameBuffer.h"
#include "math/Transformation.h"
#include "camera/PerspectiveCamera.h"
#include "scene/dynamic/DynamicScene.h"
#include "scene/dynamic/DynamicSceneNode.h"
#include "shape/TriangleMesh.h"
#include "shape/Sphere.h"
#include "shape/Plane.h"
#include "shape/Box.h"
#include "photonmapping/PhotonMapBuilder.h"
#include "preview/PreviewCanvas.h"
#include "Settings.h"
#include <emscripten/emscripten.h>
#include "api.h"

Scene buildScene(const std::string& sceneFile, float imageAspectRatio)
{
    std::cout << "Loading scene data." << std::endl;

    auto gltfScene = loadGLTFScene(sceneFile, imageAspectRatio);

    Statistics::Collector collector;
    std::cout << "Soupifying scene." << std::endl;
    gltfScene = gltfScene.soupifyScene(&collector);
    std::cout << collector.getString();
    collector.clear();

	//////

	std::cout << "Building scene." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	auto renderableScene = gltfScene.build(&collector);
	auto finish = std::chrono::high_resolution_clock::now();
	double duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
    std::cout << collector.getString();
    collector.clear();
    std::cout << "Scene build done in " << duration << " milliseconds." << std::endl;

	return renderableScene;
}

void writeOutput(Settings* settings, const FrameBuffer& buffer, const Tile& tile)
{
    if(settings->outputType == "exr")
    {
        write_to_exr_file(buffer, settings->outputPath);
    }
    else if (settings->outputType == "png")
    {
        write_to_png_file(buffer, settings->outputPath, settings->exposure, settings->gamma);
    }
    else if (settings->outputType == "ppm")
    {
        write_to_ppm(buffer, settings->outputPath);
    }
    else if (settings->outputType == "tile")
    {
        write_to_tile_file(buffer, tile, settings->outputPath);
    }
}

API_EXPORT void mergeTiles(Settings* settings, const char* tilePathsPtr)
{
    std::vector<std::string> tileFilesToMerge;

    std::string remainingStr(tilePathsPtr);
    auto splitI = std::string::npos;
    while((splitI = remainingStr.find("|")) != std::string::npos)
    {
        tileFilesToMerge.push_back(remainingStr.substr(0, splitI));
        remainingStr = remainingStr.substr(splitI+1, remainingStr.size() - splitI - 1);
    }

    Tile tile(settings->xStart, settings->yStart, settings->xEnd, settings->yEnd);

    auto buffer = std::make_shared<FrameBuffer>(settings->width, settings->height);

    if (!tileFilesToMerge.empty()) // Merge tiles
    {
        std::cout << "Merging tile files..." << std::endl;

        for (const auto& file : tileFilesToMerge)
        {
            std::cout << file << std::endl;
            try
            {
                load_from_tile_file(*buffer, file);
            }catch(const std::runtime_error& err)
            {
                std::cerr << err.what() << std::endl;
                return;
            }
        }
    }

    writeOutput(settings, *buffer, tile);
}

API_EXPORT std::shared_ptr<FrameBuffer>* newBuffer(Settings* settings)
{
    return new std::shared_ptr<FrameBuffer>(new FrameBuffer(settings->width, settings->height));
}

API_EXPORT void freeBuffer(std::shared_ptr<FrameBuffer>* buf)
{
    delete buf;
}

API_EXPORT PreviewCanvas* startDisplay(const char* canvasId, std::shared_ptr<FrameBuffer>* buf)
{
    PreviewCanvas* preview = new PreviewCanvas(*buf, std::string(canvasId));
    preview->showAsync();
    return preview;
}

API_EXPORT void stopDisplay(PreviewCanvas* window)
{
    window->stop();
    window->wait();
    delete window;
}

bool renderIsFinished = false;
API_EXPORT bool isRenderFinished()
{
    return renderIsFinished;
}

API_EXPORT void startRender(Settings* settings, std::shared_ptr<FrameBuffer>* bufPtr)
{
    renderIsFinished = false;
    std::shared_ptr<FrameBuffer> buffer = *bufPtr;

	std::filesystem::path workDirPath(settings->workDir);
	if(!std::filesystem::is_directory(workDirPath))
	{
		std::cerr << "The specified work directory does not exist or is not a folder." << std::endl;
		return;
	}

	Tile tile(settings->xStart, settings->yStart, settings->xEnd, settings->yEnd);

	auto outputfileStr = "/output.png";
	std::filesystem::path outputfile(outputfileStr);
	if(outputfile.is_relative())
	{
		outputfile = workDirPath / outputfile;
	}
	if (!outputfile.has_filename())
	{
		outputfile.replace_filename("output");
	}
	if(!outputfile.has_extension())
	{
		outputfile.replace_extension(settings->outputType);
	}

    auto sceneFile = settings->workDir + settings->sceneFile;
    if(!std::filesystem::is_regular_file(sceneFile))
    {
        std::cerr << "The specified scene file does not exist or is not a file." << std::endl;
        return;
    }

    std::filesystem::path photonMapFilePath;
    if(settings->loadPhotonMapFromFile || settings->savePhotonMapToFile)
    {
        photonMapFilePath = std::filesystem::path(settings->photonMapFile);

        if(settings->loadPhotonMapFromFile && !std::filesystem::is_regular_file(photonMapFilePath))
        {
            std::cerr << "The specified photon map file does not exist or is not a file." << std::endl;
            return;
        }
    }

    auto progressPrinter = [](const std::string& taskDesc, float progress, std::chrono::high_resolution_clock::duration duration, bool done){
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        std::cout << taskDesc << " - " << progress*100 << "% (" << ms << " ms)\n";
        if(done)
        {
            //std::cout << "\n";
        }
        std::cout.flush();
    };

    RenderSettings renderSettings;
    renderSettings.geometryAAModifier = settings->geometryAAModifier;
    renderSettings.materialAAModifier = settings->materialAAModifier;
    std::cout << "Geometry AA level = " << renderSettings.geometryAAModifier << std::endl;
    std::cout << "Material AA level = " << renderSettings.materialAAModifier << std::endl;

    std::thread thrd([sceneFile, photonMapFilePath, buffer, tile, renderSettings, progressPrinter, settings](){
        // Build scene
        std::cout << "Loading scene." << std::endl;
        auto scene = buildScene(sceneFile, static_cast<float>(settings->width)/settings->height);
        std::cout << "Scene loaded" << std::endl;

        if(settings->photonMappingMode != PhotonMapMode::none)
        {
            PhotonMap photonMap;
            if(settings->loadPhotonMapFromFile)
            {
                std::cout << "Loading photon map..." << std::endl;
                std::fstream in(photonMapFilePath, std::fstream::in | std::fstream::binary);
                photonMap = PhotonMap::deserialize(in);
                std::cout << "Loaded " << photonMap.getSize() << " photons" << std::endl;
            }
            else
            {
                std::cout << "Building photon map..." << std::endl;

                photonMap = PhotonMapBuilder::buildPhotonMap(scene, settings->photonMappingMode, progressPrinter);
            }

            if(settings->savePhotonMapToFile)
            {
                std::cout << "Saving photon map..." << std::endl;
                std::fstream out(photonMapFilePath, std::fstream::out | std::fstream::binary);
                photonMap.serialize(out);
            }

            scene.setPhotonMap(std::move(photonMap));
            scene.setPhotonMapMode(settings->photonMappingMode);
        }
        scene.setPhotonMapDepth(settings->photonMappingSampleDepth);

        std::cout << "Rendering..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        Renderer renderer;

        renderer.render(scene, *buffer, tile, renderSettings, progressPrinter, true);
        std::cout << "Done" << std::endl;

        auto finish = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
        std::cout << "Rendering time: " << duration << " milliseconds" << std::endl;

        // Write output
        std::cout << "Writing output to " << settings->outputPath << std::endl;
        writeOutput(settings, *buffer, tile);

        renderIsFinished = true;
    });
    thrd.detach();
}

int main(int argc, char** argv)
{
}

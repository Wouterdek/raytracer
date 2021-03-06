#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>
#include <filesystem>
#include <exception>

#include "io/PathResolver.h"
#include "io/PNG.h"
#include "io/EXRWriter.h"
#include "io/GLTF.h"
#include "io/TileFile.h"
#include "io/PPMFile.h"
#include "io/HDR.h"
#include "renderer/Renderer.h"
#include "film/FrameBuffer.h"
#include "scene/dynamic/DynamicScene.h"
#include "utility/MemoryUsage.h"
#include "preview/PreviewWindow.h"
#include "photonmapping/PhotonMapBuilder.h"

Scene buildScene(const std::string& sceneFile, bool soupify, float imageAspectRatio)
{
    std::cout << "Loading scene data." << std::endl;

    auto gltfScene = loadGLTFScene(sceneFile, imageAspectRatio);

    Statistics::Collector collector;
    if(soupify)
    {
        std::cout << "Soupifying scene." << std::endl;
        gltfScene = gltfScene.soupifyScene(&collector);
        std::cout << collector.getString();
        collector.clear();
    }

	//////

	std::cout << "Building scene." << std::endl;
    auto memUsageBefore = getMemoryUsage();
	auto start = std::chrono::high_resolution_clock::now();
	auto renderableScene = gltfScene.build(&collector);
	auto finish = std::chrono::high_resolution_clock::now();
    auto memUsageDelta = getMemoryUsage() - memUsageBefore;
	double duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
    std::cout << collector.getString();
    collector.clear();
    std::cout << "Scene build done in " << duration << " milliseconds. (memory delta = " << memUsageDelta << " bytes)" << std::endl;

	return renderableScene;
}

int main(int argc, char** argv)
{
	// Parse input arguments
    bool previewEnabled = false;

	namespace po = boost::program_options;
	po::options_description desc("Program options");
	desc.add_options()
		("help", "Show help")
		("workdir", po::value<std::string>()->default_value(std::filesystem::current_path().string()), "Workdir")
		("scene", po::value<std::string>()->default_value("scene.glb"), "Input GLB file")
		("width", po::value<int>()->default_value(500), "Frame width")
		("height", po::value<int>()->default_value(500), "Frame height")
		("xstart", po::value<int>()->default_value(0), "Tile start x-coordinate")
		("ystart", po::value<int>()->default_value(0), "Tile start y-coordinate")
		("xend", po::value<int>(), "Tile end x-coordinate")
		("yend", po::value<int>(), "Tile end y-coordinate")
		("exposure", po::value<double>()->default_value(-2.5), "Exposure value to apply (png only)")
		("gamma", po::value<double>()->default_value(2.2), "Gamma value to apply (png only)")
		("outputtype", po::value<std::string>()->default_value("exr"), "Type of output file to write. [exr, png, ppm, tile]")
		("output", po::value<std::string>()->default_value("output"), "Output file path")
		("mergetiles", po::value<std::vector<std::string>>()->multitoken(), "Load the specified tile files, merge them and output the result")
        ("savepm", "Write the photonmap (if any is used) to disk.")
        ("loadpm", "Load the photonmap from disk.")
        ("pmmode", po::value<std::string>()->default_value("none"), "Set the photonmapping algorithm to be used. ('none', 'caustics' or 'full')")
        ("pmdepth", po::value<int>()->default_value(0), "Set the path depth at which the photon map is used")
        ("pmfile", po::value<std::string>()->default_value(""), "The path of the photonmap file. (used for savepm and loadpm)")
        ("pmrayspointlamp", po::value<unsigned long>()->default_value(1E7), "Amount of rays to trace from each point light during photonmapping (influences, but does not equal photon count)")
        ("pmraysarealamp", po::value<unsigned long>()->default_value(1E7), "Amount of rays to trace from each area light during photonmapping (influences, but does not equal photon count)")
        ("soupify", "Use single layer BVH instead of two-layer. Results in higher memory usage and longer scene build, but might produce faster render")
        ("aageometry", po::value<int>()->default_value(4), "Geometry AA modifier: Higher means more rays from the camera")
        ("aamaterial", po::value<int>()->default_value(4), "Material AA modifier: Higher means more rays from the first hitpoint with a non-zero variance material")
        ("preview", po::bool_switch(&previewEnabled), "If enabled, a preview window will open that displays the image as its being rendered")
        ("perffci", po::value<std::string>()->default_value(""), "If set, renders an exr with performance data for each pixel at the specified path")
		;

	po::variables_map vm;
	try
	{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	}catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 0;
	}

	auto workDir = vm["workdir"].as<std::string>();
	std::filesystem::path workDirPath(workDir);
	if(!std::filesystem::is_directory(workDirPath))
	{
		std::cerr << "The specified work directory does not exist or is not a folder." << std::endl;
		return -1;
	}

	PathResolver::get().setWorkDir(workDir);

	int width = vm["width"].as<int>();
	int height = vm["height"].as<int>();

	int xstart = vm["xstart"].as<int>();
	int ystart = vm["ystart"].as<int>();
	int xend = vm.count("xend") > 0 ? vm["xend"].as<int>() : (width - xstart);
	int yend = vm.count("yend") > 0 ? vm["yend"].as<int>() : (height - ystart);
	Tile tile(xstart, ystart, xend, yend);
	if(tile.getXStart() >= tile.getXEnd() || tile.getXStart() >= width || tile.getXEnd() > width || tile.getXStart() < 0  || tile.getXEnd() < 0)
    {
        std::cerr << "Invalid xstart or xend." << std::endl;
        return -1;
    }
    if(tile.getYStart() >= tile.getYEnd() || tile.getYStart() >= height || tile.getYEnd() > height || tile.getYStart() < 0  || tile.getYEnd() <= 0)
    {
        std::cerr << "Invalid ystart or yend." << std::endl;
        return -1;
    }

	double exposure = vm["exposure"].as<double>();
	double gamma = vm["gamma"].as<double>();

	auto outputtype = vm["outputtype"].as<std::string>();
	if(outputtype != "exr" && outputtype != "png" && outputtype != "ppm" && outputtype != "tile")
	{
		throw std::runtime_error("Invalid output type: " + outputtype);
	}

	auto outputfileStr = vm["output"].as<std::string>();
	std::filesystem::path outputfile = PathResolver::get().resolve(outputfileStr);
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
		outputfile.replace_extension(outputtype);
	}

    auto perfFCIStr = vm["perffci"].as<std::string>();
	bool renderPerfFCI = !perfFCIStr.empty();
    std::filesystem::path perfFCIFile;
	if(renderPerfFCI)
    {
        perfFCIFile = PathResolver::get().resolve(perfFCIStr);
        if(perfFCIFile.is_relative())
        {
            perfFCIFile = workDirPath / perfFCIFile;
        }
        if (!perfFCIFile.has_filename())
        {
            perfFCIFile.replace_filename("perffci");
        }
        if(!perfFCIFile.has_extension())
        {
            perfFCIFile.replace_extension("exr");
        }
    }

    std::vector<std::string> tileFilesToMerge;
	if(vm.count("mergetiles") > 0){
        tileFilesToMerge = vm["mergetiles"].as<std::vector<std::string>>();
	}

    std::filesystem::path sceneFile = PathResolver::get().resolve(vm["scene"].as<std::string>());
    if(sceneFile.is_relative())
    {
        sceneFile = workDirPath / sceneFile;
    }
    if(tileFilesToMerge.size() == 0 && !std::filesystem::is_regular_file(sceneFile))
    {
        std::cerr << "The specified scene file does not exist or is not a file. (" << sceneFile << ")" << std::endl;
        return -1;
    }

    bool loadPhotonMapFromFile = vm.count("loadpm") > 0;
    bool savePhotonMapToFile = vm.count("savepm") > 0;
    std::filesystem::path photonMapFilePath;
    if(loadPhotonMapFromFile || savePhotonMapToFile)
    {
        auto photonMapFile = vm["pmfile"].as<std::string>();
        photonMapFilePath = PathResolver::get().resolve(photonMapFile);

        if(loadPhotonMapFromFile && !std::filesystem::is_regular_file(photonMapFilePath))
        {
            std::cerr << "The specified photon map file does not exist or is not a file." << std::endl;
            return -1;
        }
    }

    int pmdepth = vm["pmdepth"].as<int>();

    PhotonMapMode photonMappingMode = PhotonMapMode::none;
    const auto& pmmodeString = vm["pmmode"].as<std::string>();
    if(pmmodeString == "caustics")
    {
        photonMappingMode = PhotonMapMode::caustics;
    }
    else if(pmmodeString == "full")
    {
        photonMappingMode = PhotonMapMode::full;
    }
    else if(pmmodeString == "none")
    {}
    else
    {
        std::cerr << "Invalid photonmapping mode!" << std::endl;
        return -1;
    }

    int aageometry = vm["aageometry"].as<int>();
    int aamaterial = vm["aamaterial"].as<int>();
    if(aageometry <= 0 || aamaterial <= 0)
    {
        std::cerr << "Invalid AA settings!" << std::endl;
        return -1;
    }

    unsigned long pmrayspointlamp = vm["pmrayspointlamp"].as<unsigned long>();
    unsigned long pmraysarealamp = vm["pmraysarealamp"].as<unsigned long>();
    if(pmrayspointlamp <= 0 || pmraysarealamp <= 0)
    {
        std::cerr << "Invalid PM raycount settings!" << std::endl;
        return -1;
    }

	// Create picture

	auto buffer = std::make_shared<FrameBuffer>(width, height);

    std::shared_ptr<FrameBuffer> perfFCI = nullptr;
    if(renderPerfFCI)
    {
        perfFCI = std::make_shared<FrameBuffer>(width, height);
    }

    PreviewWindow previewWindow(buffer);
    if(previewEnabled)
    {
        previewWindow.showAsync();
    }

	if (!tileFilesToMerge.empty()) // Merge tiles
	{
		std::cout << "Merging tile files..." << std::endl;

		for (const auto& file : tileFilesToMerge)
		{
            auto filePath = PathResolver::get().resolve(file);
			std::cout << filePath << std::endl;
			try
			{
				load_from_tile_file(*buffer, filePath);
			}catch(const std::runtime_error& err)
			{
				std::cerr << err.what() << std::endl;
				return -1;
			}
		}
	}
	else // Render
	{
		// Build scene
        std::cout << "Loading scene." << std::endl;
        auto memUsageBefore = getMemoryUsage();
		auto scene = buildScene(sceneFile, vm.count("soupify"), static_cast<float>(width)/height);
        auto memUsageDelta = getMemoryUsage() - memUsageBefore;
        std::cout << "Scene loaded, total memory delta = " << memUsageDelta << " bytes" << std::endl;

        auto progressPrinter = [](const std::string& taskDesc, float progress, std::chrono::high_resolution_clock::duration duration, bool done){
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            std::cout << taskDesc << " - " << progress*100 << "% (" << ms << " ms)\r";
            if(done)
            {
                std::cout << "\n";
            }
            std::cout.flush();
        };
        if(photonMappingMode != PhotonMapMode::none)
        {
            PhotonMap photonMap;
            if(loadPhotonMapFromFile)
            {
                std::cout << "Loading photon map..." << std::endl;
                std::fstream in(photonMapFilePath, std::fstream::in | std::fstream::binary);
                photonMap = PhotonMap::deserialize(in);
                std::cout << "Loaded " << photonMap.getSize() << " photons" << std::endl;
            }
            else
            {
                std::cout << "Building photon map..." << std::endl;

                photonMap = PhotonMapBuilder::buildPhotonMap(scene, photonMappingMode, pmraysarealamp, pmrayspointlamp, progressPrinter);
            }

            if(savePhotonMapToFile)
            {
                std::cout << "Saving photon map..." << std::endl;
                std::fstream out(photonMapFilePath, std::fstream::out | std::fstream::binary);
                photonMap.serialize(out);
            }

            scene.setPhotonMap(std::move(photonMap));
            scene.setPhotonMapMode(photonMappingMode);
        }
        scene.setPhotonMapDepth(pmdepth);

		auto start = std::chrono::high_resolution_clock::now();

		std::cout << "Rendering..." << std::endl;

		RenderSettings settings;
		settings.geometryAAModifier = aageometry;
        settings.materialAAModifier = aamaterial;
        std::cout << "Geometry AA level = " << settings.geometryAAModifier << std::endl;
        std::cout << "Material AA level = " << settings.materialAAModifier << std::endl;
		//PPMRenderer renderer;
		Renderer renderer;
		renderer.render(scene, *buffer, perfFCI, tile, settings, progressPrinter, true);
        std::cout << "Done" << std::endl;

		auto finish = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
		std::cout << "Rendering time: " << duration << " milliseconds" << std::endl;
	}
	
	// Write output
	std::filesystem::create_directories(outputfile.parent_path());
	std::cout << "Writing output to " << outputfile.string() << std::endl;
	if(outputtype == "exr")
	{
		write_to_exr_file(*buffer, outputfile.string(), false);
	}
	else if (outputtype == "png")
	{
		write_to_png_file(*buffer, outputfile.string(), exposure, gamma);
	}
	else if (outputtype == "ppm")
	{
		write_to_ppm(*buffer, outputfile.string());
	}
	else if (outputtype == "tile")
	{
		write_to_tile_file(*buffer, tile, outputfile.string());
	}

	if(renderPerfFCI)
    {
        write_to_exr_file(*perfFCI, perfFCIFile.string(), true);
    }

    previewWindow.wait();

	return 0;
}

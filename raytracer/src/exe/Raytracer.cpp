﻿#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>
#include <filesystem>
#include <exception>

#include "film/FrameBuffer.h"
#include "io/PPMFile.h"
#include "math/Transformation.h"
#include "shape/Sphere.h"
#include "shape/Plane.h"
#include "camera/PerspectiveCamera.h"
#include "material/DiffuseMaterial.h"

#include "scene/dynamic/DynamicScene.h"
#include "scene/dynamic/DynamicSceneNode.h"
#include "shape/TriangleMesh.h"
#include "renderer/Renderer.h"

#include "shape/Box.h"
#include "material/NormalMaterial.h"

#include "material/TexCoordMaterial.h"
#include "io/PNGWriter.h"
#include "io/EXRWriter.h"
#include "io/gltf/gltf.h"
#include "io/OBJLoader.h"
#include "io/TileFile.h"

Scene buildScene(std::string workDir)
{
	Point origin(0, 1.5, 0);
	Point destination(0, 1.5, 1);
	Vector3 lookup(0, 1, 0);
	double fov = 90;

	auto normalMat = std::make_shared<NormalMaterial>();
	auto texCoordMat = std::make_shared<TexCoordMaterial>();

	auto diffuseMat = std::make_shared<DiffuseMaterial>();
	diffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	diffuseMat->ambientIntensity = 0.01;
	diffuseMat->diffuseColor = RGB{ 1.0, 1.0, 0.8 };
	diffuseMat->diffuseIntensity = 1.0;

	auto redDiffuseMat = std::make_shared<DiffuseMaterial>();
	redDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	redDiffuseMat->ambientIntensity = 0.01;
	redDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	redDiffuseMat->diffuseIntensity = 1.0;

	auto whiteMat = std::make_shared<DiffuseMaterial>();
	whiteMat->ambientColor = RGB{ 1.0, 1.0, 1.0 };
	whiteMat->ambientIntensity = 1000;
	whiteMat->diffuseColor = RGB{ 1.0, 1.0, 1.0 };
	whiteMat->diffuseIntensity = 1.0;

	Model sphereModel(std::make_shared<Sphere>(), diffuseMat);

	/*Model triangleModel{};
	std::vector<Point> triangleVertices = { Point(0.0, 0.0, 0.0), Point(1.0, 0.0, 0.0), Point(0.0, 1.0, 0.0) };
	std::vector<uint32_t> triangleIndices = { 0, 1, 2 };
	std::vector<Vector3> triangleNormals = { Vector3(0.0, 0.0, 1.0), Vector3(0.0, 0.0, 1.0), Vector3(0.0, 0.0, 1.0) };
	std::vector<uint32_t> triangleNormalIndices = { 0, 1, 2 };
	triangleModel.shape = std::make_shared<TriangleMesh>(triangleVertices, triangleIndices, triangleNormals, triangleNormalIndices);
	triangleModel.material = redDiffuseMat;*/

	Model bunnyModel(std::make_shared<TriangleMesh>(loadOBJMesh(workDir + "/models/bunny_low.obj")), redDiffuseMat);
	Model triangleModel(std::make_shared<TriangleMesh>(loadOBJMesh(workDir + "/models/triangle.obj")), whiteMat);

	//Model dragonModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/dragon_high.obj")), normalMat);

	//Model lucyModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/lucy.obj")), redDiffuseMat);

	//Model asianDragonModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/xyzrgb_dragon.obj")), redDiffuseMat);


	/*auto amDiffuseMat = std::make_shared<DiffuseMaterial>();
	amDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	amDiffuseMat->ambientIntensity = 0.7;
	amDiffuseMat->albedo = std::make_shared<Texture>(Texture::load(workDir + "/models/autumn_casualwoman/autumn_casualwoman_02_-diffuse.png"));
	amDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	amDiffuseMat->diffuseIntensity = 1.0;
	Model amModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/autumn_casualwoman/autumn_casualwoman_02_highpoly.OBJ")), amDiffuseMat);*/

	/*auto coffeeDiffuseMat = std::make_shared<DiffuseMaterial>();
	coffeeDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	coffeeDiffuseMat->ambientIntensity = 0.7;
	coffeeDiffuseMat->albedoMap = std::make_shared<Texture>(Texture::load(workDir + "/models/coffee/ark_coffee.png"));
	coffeeDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	coffeeDiffuseMat->diffuseIntensity = 1.0;
	Model coffeeModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/coffee/ARK_COFFEE_CUP.obj")), coffeeDiffuseMat);*/

	/*auto appleDiffuseMat = std::make_shared<DiffuseMaterial>();
	appleDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	appleDiffuseMat->ambientIntensity = 0.7;
	appleDiffuseMat->albedoMap = std::make_shared<Texture>(Texture::load(workDir + "/models/apple/apple_texture.png"));
	appleDiffuseMat->normalMap = std::make_shared<Texture>(Texture::load(workDir + "/models/apple/apple_normal.png"));
	appleDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	appleDiffuseMat->diffuseIntensity = 1.0;
	Model appleModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/apple/apple.obj")), appleDiffuseMat);*/

	/*auto handDiffuseMat = std::make_shared<DiffuseMaterial>();
	handDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	handDiffuseMat->ambientIntensity = 0.7;
	handDiffuseMat->albedoMap = std::make_shared<Texture>(Texture::load(workDir + "/models/hand/HAND_C.png"));
	handDiffuseMat->normalMap = std::make_shared<Texture>(Texture::load(workDir + "/models/hand/HAND_N.png"));
	handDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	handDiffuseMat->diffuseIntensity = 1.0;
	Model handModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/hand/uneedahand.obj")), handDiffuseMat);*/

	//Model teapotModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/teapot.obj")), normalMat);

	Model planeModel(std::make_shared<Plane>(), diffuseMat);

	//Model boxModel(std::make_shared<Box>(Vector3(0, 0, 0), Vector3(1, 1, 1)), redDiffuseMat);

	//Model planeMeshModel(std::make_shared<TriangleMesh>(loadMesh(workDir + "/models/plane.obj")), redDiffuseMat);


	////////

	DynamicScene scene{};

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 0, 10).append(Transformation::scale(5, 5, 5));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 0, 10).append(Transformation::rotateX(-10)).append(Transformation::scale(2, 2, 2));
	curNode->model = std::make_unique<Model>(planeMeshModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	Transformation dragonTransform = Transformation::translate(0, -5, 10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));;
	curNode->transform = dragonTransform;
	curNode->model = std::make_unique<Model>(dragonModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	Transformation lucyTransform = Transformation::translate(0, -5, 10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));;
	curNode->transform = lucyTransform;
	curNode->model = std::make_unique<Model>(lucyModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, 10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(asianDragonModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -2, 10).append(Transformation::rotateY(90)).append(Transformation::scale(4, 4, 4));
	curNode->model = std::make_unique<Model>(appleModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -7, 15).append(Transformation::rotateY(90)).append(Transformation::scale(60, 60, 60));
	curNode->model = std::make_unique<Model>(handModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(0, 0, 7).append(Transformation::rotateY(0)).append(Transformation::scale(1.5, 1.5, 1.5));
		curNode->model = std::make_unique<Model>(bunnyModel);
		scene.root->children.emplace_back(std::move(curNode));
	}*/

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(3, 0, 9).append(Transformation::rotateY(0));
		curNode->model = std::make_unique<Model>(bunnyModel);
		scene.root->children.emplace_back(std::move(curNode));
	}

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(1, 0, 10).append(Transformation::rotateY(0));
		curNode->model = std::make_unique<Model>(bunnyModel);
		scene.root->children.emplace_back(std::move(curNode));
	}

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(-1, 0, 11).append(Transformation::rotateY(0));
		curNode->model = std::make_unique<Model>(bunnyModel);
		scene.root->children.emplace_back(std::move(curNode));
	}

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(-3, 0, 12).append(Transformation::rotateY(0));
		curNode->model = std::make_unique<Model>(bunnyModel);
		scene.root->children.emplace_back(std::move(curNode));
	}

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(-5, 0, 13).append(Transformation::rotateY(0));
		curNode->model = std::make_unique<Model>(bunnyModel);
		scene.root->children.emplace_back(std::move(curNode));
	}

	/*
	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(2, -5, 14).append(Transformation::scale(6, 6, 6));
		curNode->model = std::make_unique<Model>(teapotModel);
		scene.root->children.emplace_back(std::move(curNode));
	}*/

	/*curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 0, 8).append(Transformation::scale(0.5, 0.5, 0.5));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(3, 0, 20).append(Transformation::rotateX(30)).append(Transformation::scale(3, 3, 3));
	curNode->model = std::make_unique<Model>(boxModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, 6).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(triangleModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(0, 0, 0);
		curNode->model = std::make_unique<Model>(planeModel);
		scene.root->children.emplace_back(std::move(curNode));
	}

	/*{
		std::random_device rd;
		std::uniform_real_distribution<float> dist(0, 1);
		auto parentNode = std::make_unique<DynamicSceneNode>();
		parentNode->transform = Transformation::translate(-10, 0, -30).append(Transformation::rotateX(85));
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				auto curNode = std::make_unique<DynamicSceneNode>();
				curNode->transform = Transformation::translate((i * 2), j * 2, -dist(rd));
				curNode->model = std::make_unique<Model>(sphereModel);
				parentNode->children.emplace_back(std::move(curNode));
			}
		}
		scene.root->children.emplace_back(std::move(parentNode));
	}*/

	/*std::random_device rd;
	std::uniform_real_distribution<> dist(-100, -5);
	for(int i = 0; i < sphereCount; i++)
	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		float z = dist(rd);
		curNode->transform = Transformation::translate(0, 6, z);
		curNode->model = std::make_unique<Model>(sphereModel);
		scene.root->children.emplace_back(std::move(curNode));
	}*/

	/*{
		auto curNode = std::make_unique<DynamicSceneNode>();
		//curNode->transform = Transformation::translate(-10, 10, -15);
		curNode->transform = Transformation::translate(1, 1, 0);
		curNode->pointLight = std::make_unique<PointLight>();
		//curNode->pointLight->intensity *= 10;
		scene.root->children.emplace_back(std::move(curNode));
	}*/

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(5, 0, 17).append(Transformation::rotateY(225)).append(Transformation::scale(20, 20, 20));
		curNode->areaLight = std::make_unique<AreaLight>();
		//curNode->model = std::make_unique<Model>(triangleModel);
		scene.root->children.emplace_back(std::move(curNode));
	}

	/*curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(-10, 11, -15).append(Transformation::scale(0.1, 0.1, 0.1));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*
	Transformation t3 =
	Transformation t4 = Transformation::translate(4, 4, -12).append(Transformation::scale(4, 4, 4));
	Transformation t5 = Transformation::translate(-4, 4, -12).append(Transformation::scale(4, 4, 4));
	*/
	{
		auto cameraNode = std::make_unique<DynamicSceneNode>();
		//cameraNode.transform = 
		//cameraNode.camera->pointAt(destination, lookup);
		cameraNode->camera = std::make_unique<PerspectiveCamera>(fov);
		cameraNode->transform = Transformation::translate(origin).append(Transformation::lookat(destination - origin, lookup));
		scene.root->children.emplace_back(std::move(cameraNode));
	}
	

	//auto gltfScene = loadGLTFScene(workDir + "/models/kitchen/kitchen.glb");

	/*{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(-11.916, .28301, .34278).append(Transformation::rotateY(90)).append(Transformation::scale(7.5, 7.5, 7.5));
		curNode->areaLight = std::make_unique<AreaLight>();
		//curNode->model = std::make_unique<Model>(triangleModel);
		gltfScene.root->children.emplace_back(std::move(curNode));
	}*/

	/*{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(-11.916, .28301, .34278).append(Transformation::rotateY(90));
		curNode->pointLight = std::make_unique<PointLight>();
		curNode->pointLight->intensity *= 10;
		gltfScene.root->children.emplace_back(std::move(curNode));
	}*/
	/*DynamicSceneNode& cameraNode2 = *(gltfScene.root->children.emplace_back(std::make_unique<DynamicSceneNode>()));
	cameraNode2.camera = std::make_unique<PerspectiveCamera>(width, height, origin, destination, lookup, fov);*/
	//auto gltfSceneRenderable = gltfScene.build();

	//////

	std::cout << "Building scene." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	auto renderableScene = scene.build();
	auto finish = std::chrono::high_resolution_clock::now();
	double duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
	std::cout << "Scene build done in " << duration << " milliseconds." << std::endl;
	
	return renderableScene;
}

int main(int argc, char** argv)
{
	// Parse input arguments
	namespace po = boost::program_options;
	po::options_description desc("Program options");
	desc.add_options()
		("help", "Show help")
		("workdir", po::value<std::string>()->default_value(std::filesystem::current_path().string()), "Workdir")
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
	}

	int width = vm["width"].as<int>();
	int height = vm["height"].as<int>();

	int xstart = vm["xstart"].as<int>();
	int ystart = vm["ystart"].as<int>();
	int xend = vm.count("xend") > 0 ? vm["xend"].as<int>() : (width - xstart);
	int yend = vm.count("yend") > 0 ? vm["yend"].as<int>() : (height - ystart);
	Tile tile(xstart, ystart, xend, yend);

	double exposure = vm["exposure"].as<double>();
	double gamma = vm["gamma"].as<double>();

	auto outputtype = vm["outputtype"].as<std::string>();
	if(outputtype != "exr" && outputtype != "png" && outputtype != "ppm" && outputtype != "tile")
	{
		throw std::runtime_error("Invalid output type: " + outputtype);
	}

	auto outputfileStr = vm["output"].as<std::string>();
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
		outputfile.replace_extension(outputtype);
	}

    std::vector<std::string> tileFilesToMerge;
	if(vm.count("mergetiles") > 0){
        tileFilesToMerge = vm["mergetiles"].as<std::vector<std::string>>();
	}

	// Create picture

	FrameBuffer buffer(width, height);

	if (tileFilesToMerge.size() > 0) // Merge tiles
	{
		std::cout << "Merging tile files..." << std::endl;

		for (const auto& file : tileFilesToMerge)
		{
			std::cout << file << std::endl;
			try
			{
				load_from_tile_file(buffer, file);
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
		auto scene = buildScene(workDir);

		auto start = std::chrono::high_resolution_clock::now();

		std::cout << "Rendering..." << std::endl;

		RenderSettings settings;
		settings.aaLevel = 8;
		render(scene, buffer, tile, settings);

		auto finish = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
		std::cout << "Rendering time: " << duration << " milliseconds" << std::endl;
	}
	
	// Write output
	std::cout << "Writing output to " << outputfile.string() << std::endl;
	if(outputtype == "exr")
	{
		write_to_exr_file(buffer, outputfile.string());
	}
	else if (outputtype == "png")
	{
		write_to_png_file(buffer, outputfile.string(), exposure, gamma);
	}
	else if (outputtype == "ppm")
	{
		write_to_ppm(buffer, outputfile.string());
	}
	else if (outputtype == "tile")
	{
		write_to_tile_file(buffer, tile, outputfile.string());
	}

	return 0;
}

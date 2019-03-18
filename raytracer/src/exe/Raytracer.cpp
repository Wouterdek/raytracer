#include <math/ray.h>
#include <random>
#include <chrono>
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

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "io/tiny_obj_loader.h"
#include "shape/Box.h"
#include "material/NormalMaterial.h"
#include <iostream>

TriangleMesh loadMesh(std::string path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str());

	if(ret)
	{
		std::vector<Point> vertices;
		vertices.reserve(attrib.vertices.size() / 3);
		
		for(int i = 0; i < attrib.vertices.size(); i += 3)
		{
			vertices.emplace_back(attrib.vertices[i], attrib.vertices[i+1], attrib.vertices[i+2]);
		}

		std::vector<Vector3> normals;
		normals.reserve(attrib.normals.size() / 3);

		for (int i = 0; i < attrib.normals.size(); i += 3)
		{
			normals.emplace_back(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]);
		}

		std::vector<Vector2> texCoords;
		texCoords.reserve(attrib.texcoords.size() / 2);

		for (int i = 0; i < attrib.texcoords.size(); i += 2)
		{
			texCoords.emplace_back(attrib.texcoords[i], attrib.texcoords[i + 1]);
		}

		std::vector<std::array<uint32_t, 3>> indices;
		std::vector<std::array<uint32_t, 3>> normalIndices;
		std::vector<std::array<uint32_t, 3>> texCoordIndices;
		auto& objIndices = shapes[0].mesh.indices;
		indices.reserve(objIndices.size() / 3);
		normalIndices.reserve(objIndices.size() / 3);
		texCoordIndices.reserve(objIndices.size() / 3);
		for (int i = 0; i < objIndices.size(); i += 3)
		{
			indices.emplace_back<std::array<uint32_t, 3>>({
				static_cast<uint32_t>(objIndices[i].vertex_index),
				static_cast<uint32_t>(objIndices[i+1].vertex_index),
				static_cast<uint32_t>(objIndices[i+2].vertex_index)
			});
			normalIndices.emplace_back<std::array<uint32_t, 3>>({
				static_cast<uint32_t>(objIndices[i].normal_index),
				static_cast<uint32_t>(objIndices[i + 1].normal_index),
				static_cast<uint32_t>(objIndices[i + 2].normal_index)
			});
			texCoordIndices.emplace_back<std::array<uint32_t, 3>>({
				static_cast<uint32_t>(objIndices[i].texcoord_index),
				static_cast<uint32_t>(objIndices[i + 1].texcoord_index),
				static_cast<uint32_t>(objIndices[i + 2].texcoord_index)
			});
		}

		return TriangleMesh(vertices, indices, normals, normalIndices, texCoords, texCoordIndices);
	}

	throw std::exception("bad file");
}

void render(const Scene& scene, FrameBuffer& buffer, const Tile& tile)
{
	const ICamera& camera = scene.getCameras()[0].getData();

	double maxVal;

	for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
		for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
			// create a ray through the center of the pixel.
			Ray ray = camera.generateRay(Sample(x + 0.5, y + 0.5));

			// test the scene on intersections
			//auto start = std::chrono::high_resolution_clock::now();

			auto hit = scene.traceRay(ray);

			//auto finish = std::chrono::high_resolution_clock::now();
			//double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count()/1000.0;

			//buffer.setPixel(x, y, RGB(duration));
			//maxVal = std::max(maxVal, duration);

			//RGB bvhMarker(0, std::max(0.0, ((double)BVHDiag::Levels)) / 50.0, 0);
			//BVHDiag::Levels = 0;

			// add a color contribution to the pixel
			if (hit.has_value())
			{
				/*auto hitpoint = hit->getGeometryInfo().getHitpoint();
				auto depth = std::log(hitpoint.norm())/4.0f;
				depth = std::clamp<float>(depth, 0, 1);
				buffer.setPixel(x, y, RGB(depth));*/
				
				buffer.setPixel(x, y, hit->getModelNode().getData().getMaterial().getColorFor(*hit, scene, 0));
			}
			//buffer.setPixel(x, y, bvhMarker.add(buffer.getPixel(x, y)));
		}

		std::cout << y * 100 / tile.getYEnd() << "% done\r";
	}

	/*for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
		for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
			buffer.setPixel(x, y, buffer.getPixel(x, y).divide(maxVal));
		}
	}*/
}

void render(std::string filename)
{
	int width = 1000;
	int height = 1000;
	double sensitivity = 1.0;
	double gamma = 2.2;
	bool quiet = false;
	Point origin(0, 0, 0);
	Point destination(0, 0, -1);
	Vector3 lookup(0, 1, 0);
	double fov = 90;

	//////

	auto normalMat = std::make_shared<NormalMaterial>();

	auto diffuseMat = std::make_shared<DiffuseMaterial>();
	diffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	diffuseMat->ambientIntensity = 0.5;
	diffuseMat->diffuseColor = RGB{ 1.0, 1.0, 0.8 };
	diffuseMat->diffuseIntensity = 1.0;

	auto redDiffuseMat = std::make_shared<DiffuseMaterial>();
	redDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	redDiffuseMat->ambientIntensity = 0.7;
	redDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	redDiffuseMat->diffuseIntensity = 1.0;

	Model sphereModel(std::make_shared<Sphere>(), diffuseMat);

	/*Model triangleModel{};
	std::vector<Point> triangleVertices = { Point(0.0, 0.0, 0.0), Point(1.0, 0.0, 0.0), Point(0.0, 1.0, 0.0) };
	std::vector<uint32_t> triangleIndices = { 0, 1, 2 };
	std::vector<Vector3> triangleNormals = { Vector3(0.0, 0.0, 1.0), Vector3(0.0, 0.0, 1.0), Vector3(0.0, 0.0, 1.0) };
	std::vector<uint32_t> triangleNormalIndices = { 0, 1, 2 };
	triangleModel.shape = std::make_shared<TriangleMesh>(triangleVertices, triangleIndices, triangleNormals, triangleNormalIndices);
	triangleModel.material = redDiffuseMat;*/

	//Model bunnyModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/bunny_low.obj")), redDiffuseMat);

	//Model dragonModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/dragon_high.obj")), redDiffuseMat);

	//Model lucyModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/lucy.obj")), redDiffuseMat);

	//Model asianDragonModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/xyzrgb_dragon.obj")), redDiffuseMat);


	auto amDiffuseMat = std::make_shared<DiffuseMaterial>();
	amDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	amDiffuseMat->ambientIntensity = 0.7;
	amDiffuseMat->albedo = std::make_shared<Texture>(Texture::load("F:/models/autumn_casualwoman/autumn_casualwoman_02_-diffuse.png"));
	amDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	amDiffuseMat->diffuseIntensity = 1.0;
	Model amModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/autumn_casualwoman/autumn_casualwoman_02_highpoly.OBJ")), redDiffuseMat);

	//Model teapotModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/teapot.obj")), normalMat);

	Model planeModel(std::make_shared<Plane>(), diffuseMat);

	//Model boxModel(std::make_shared<Box>(Vector3(0, 0, 0), Vector3(1, 1, 1)), redDiffuseMat);

	//Model planeMeshModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/plane.obj")), redDiffuseMat);


	////////

	DynamicScene scene{};

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 0, -10).append(Transformation::scale(5, 5, 5));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 0, -10).append(Transformation::rotateX(-10)).append(Transformation::scale(2, 2, 2));
	curNode->model = std::make_unique<Model>(planeMeshModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	Transformation dragonTransform = Transformation::translate(0, -5, -10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));;
	curNode->transform = dragonTransform;
	curNode->model = std::make_unique<Model>(dragonModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	Transformation lucyTransform = Transformation::translate(0, -5, -10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));;
	curNode->transform = lucyTransform;
	curNode->model = std::make_unique<Model>(lucyModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(asianDragonModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(1, -3, -12).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(amModel);
	scene.root->children.emplace_back(std::move(curNode));

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -10).append(Transformation::rotateY(0)).append(Transformation::scale(4, 4, 4));
	curNode->model = std::make_unique<Model>(bunnyModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -10).append(Transformation::rotateX(90)).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(teapotModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 0, -8).append(Transformation::scale(0.5, 0.5, 0.5));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(3, 0, -20).append(Transformation::rotateX(30)).append(Transformation::scale(3, 3, 3));
	curNode->model = std::make_unique<Model>(boxModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -6).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(triangleModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(0, -11.2, 0);
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

	{
		auto curNode = std::make_unique<DynamicSceneNode>();
		//curNode->transform = Transformation::translate(-10, 10, -15);
		curNode->transform = Transformation::translate(0, 5, -3);
		curNode->lamp = std::make_unique<PointLamp>();
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

	DynamicSceneNode& cameraNode = *(scene.root->children.emplace_back(std::make_unique<DynamicSceneNode>()));
	cameraNode.camera = std::make_unique<PerspectiveCamera>(width, height, origin, destination, lookup, fov);
	auto& camera = *(cameraNode.camera);

	//////

	std::cout << "Building scene." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	auto renderableScene = scene.build();
	auto finish = std::chrono::high_resolution_clock::now();
	double duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
	std::cout << "Done in " << duration << " milliseconds." << std::endl;

	FrameBuffer buffer(width, height);

	//

	// subdivide the buffer in equal sized tiles
	std::cout << "Rendering..." << std::endl;
	for (auto tile : buffer.subdivide(width, height)) {
		render(renderableScene, buffer, tile);
	}

	/*
	auto time = finish - start;
	std::cout << "sphere count: " << sphereCount << " time: " << time.count() << std::endl;*/

	write_to_ppm(buffer, filename);
}

int main(char** argc, int argv)
{
	/*for (int k = 0; k < 50; k++) {
		for (int i = 1; i < 10001; i *= 10) {
			std::ostringstream s;
			s << "C:/Users/Wouter/Desktop/render/" << i << ".ppm";
			render(i, s.str());
		}
		std::cout << std::endl;
	}
	std::cin.get();*/
	//render("C:/Users/Wouter/Desktop/render/dragon_normal.ppm");
	render("C:/Users/Wouter/Desktop/render/render.ppm");
	//render("C:/Users/Wouter/Desktop/render/sphere.ppm");
}

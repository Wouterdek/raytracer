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
			vertices.push_back(Point{ attrib.vertices[i], attrib.vertices[i+1], attrib.vertices[i+2] });
		}

		std::vector<Vector3> normals;
		normals.reserve(attrib.normals.size() / 3);

		for (int i = 0; i < attrib.normals.size(); i += 3)
		{
			normals.push_back(Vector3{ attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2] });
		}

		std::vector<std::array<uint32_t, 3>> indices;
		std::vector<std::array<uint32_t, 3>> normalIndices;
		auto& objIndices = shapes[0].mesh.indices;
		auto& objNormalIndices = shapes[0].mesh.indices;
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
		}

		return TriangleMesh(vertices, indices, normals, normalIndices);
	}

	throw std::exception("bad file");
}

void render(const Scene& scene, FrameBuffer& buffer, const Tile& tile)
{
	const ICamera& camera = scene.getCameras()[0].getData();

	for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
		for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
			// create a ray through the center of the pixel.
			Ray ray = camera.generateRay(Sample(x + 0.5, y + 0.5));

			// test the scene on intersections
			auto hit = scene.traceRay(ray);

			// add a color contribution to the pixel
			if (hit.has_value())
			{
				//auto hitpoint = hit->first.getHitpoint();
				//auto depth = std::log(hitpoint.norm())/4.0f;
				//buffer.setPixel(x, y, RGB(depth));

				buffer.setPixel(x, y, hit->getModelNode().getData().getMaterial().getColorFor(hit->getGeometryInfo(), scene, 0));
			}
		}
	}
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

	Model dragonModel(std::make_shared<TriangleMesh>(loadMesh("C:/Users/Wouter/Desktop/dragon_low.obj")), normalMat);

	Model teapotModel(std::make_shared<TriangleMesh>(loadMesh("C:/Users/Wouter/Desktop/teapot.obj")), redDiffuseMat);

	Model planeModel(std::make_shared<Plane>(), diffuseMat);

	Model boxModel(std::make_shared<Box>(Vector3(0, 0, 0), Vector3(1, 1, 1)), redDiffuseMat);


	////////

	DynamicScene scene{};

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 0, -10).append(Transformation::scale(5, 5, 5));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(dragonModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -10).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(teapotModel);
	scene.root->children.emplace_back(std::move(curNode));

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, 6, -18).append(Transformation::scale(2, 2, 2));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -20).append(Transformation::scale(3, 3, 3));
	curNode->model = std::make_unique<Model>(boxModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, -6).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(triangleModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -5, 0);
	curNode->model = std::make_unique<Model>(planeModel);
	scene.root->children.emplace_back(std::move(curNode));

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

	curNode = std::make_unique<DynamicSceneNode>();
	//curNode->transform = Transformation::translate(-10, 10, -15);
	curNode->transform = Transformation::translate(-5, 12, -5);
	curNode->lamp = std::make_unique<PointLamp>();
	scene.root->children.emplace_back(std::move(curNode));

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

	auto renderableScene = scene.build();

	FrameBuffer buffer(width, height);

	//auto start = std::chrono::high_resolution_clock::now();

	// subdivide the buffer in equal sized tiles
	for (auto tile : buffer.subdivide(64, 64)) {
		render(renderableScene, buffer, tile);
	}

	/*auto finish = std::chrono::high_resolution_clock::now();
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
	render("C:/Users/Wouter/Desktop/render/tea.ppm");
	//render("C:/Users/Wouter/Desktop/render/sphere.ppm");
}

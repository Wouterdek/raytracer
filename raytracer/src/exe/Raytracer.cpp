#include "raytracer.h"
#include <math/ray.h>
#include "film/FrameBuffer.h"
#include "io/PPMFile.h"
#include "math/Transformation.h"
#include "shape/Sphere.h"
#include "shape/Plane.h"
#include "camera/PerspectiveCamera.h"
#include "material/DiffuseMaterial.h"

#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "shape/TriangleMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "io/tiny_obj_loader.h"
#include "shape/Box.h"

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

		std::vector<uint32_t> indices;
		std::vector<uint32_t> normalIndices;
		auto& objIndices = shapes[0].mesh.indices;
		auto& objNormalIndices = shapes[0].mesh.indices;
		for (int i = 0; i < objIndices.size(); i++)
		{
			indices.push_back(static_cast<uint32_t>(objIndices[i].vertex_index));
			normalIndices.push_back(static_cast<uint32_t>(objNormalIndices[i].normal_index));
		}

		return TriangleMesh(vertices, indices, normals, normalIndices);
	}

	throw std::exception("bad file");
}

int main(char** argc, int argv)
{
	int width = 640;
	int height = 640;
	double sensitivity = 1.0;
	double gamma = 2.2;
	bool quiet = false;
	Point origin(0, 0, 0);
	Point destination(0, 0, -1);
	Vector3 lookup(0, 1, 0);
	double fov = 90;
	std::string filename("C:/Users/Wouter/Desktop/img.ppm");

	//////

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

	Model sphereModel {};
	sphereModel.shape = std::make_shared<Sphere>();
	sphereModel.material = diffuseMat;

	/*Model triangleModel {};
	std::vector<Point> triangleVertices = { Point(0.0, 0.0, 0.0), Point(1.0, 0.0, 0.0), Point(0.0, 0.0, -1.0) };
	std::vector<uint32_t> triangleIndices = { 0, 1, 2 };
	std::vector<Vector3> triangleNormals = { Vector3(0.0, 0.0, 1.0), Vector3(0.0, 0.0, 1.0), Vector3(0.0, 0.0, 1.0) };
	triangleModel.shape = std::make_shared<TriangleMesh>(triangleVertices, triangleIndices, triangleNormals);
	triangleModel.material = redDiffuseMat;*/

	Model dragonModel{};
	dragonModel.shape = std::make_shared<TriangleMesh>(loadMesh("C:/Users/Wouter/Desktop/dragon_low.obj"));
	dragonModel.material = redDiffuseMat;

	Model planeModel{};
	planeModel.shape = std::make_shared<Plane>();
	planeModel.material = diffuseMat;

	Model boxModel{};
	boxModel.shape = std::make_shared<Box>(Vector3(1, 1, 1));
	boxModel.material = diffuseMat;


	////////

	Scene scene{};

	/*auto curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(5, 3, -20).append(Transformation::scale(5, 5, 5));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/
	
	/*auto curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(0, -4, -10).append(Transformation::rotateY(90)).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(dragonModel);
	scene.root->children.emplace_back(std::move(curNode));*/
	/*
	curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(0, 6, -18).append(Transformation::scale(2, 2, 2));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	auto curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(0, -4, -10).append(Transformation::scale(6, 6, 6));
	curNode->model = std::make_unique<Model>(boxModel);
	scene.root->children.emplace_back(std::move(curNode));

	curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(0, -5, 0);
	curNode->model = std::make_unique<Model>(planeModel);
	scene.root->children.emplace_back(std::move(curNode));

	curNode = std::make_unique<SceneNode>();
	//curNode->transform = Transformation::translate(-10, 10, -15);
	curNode->transform = Transformation::translate(-5, 12, -5);
	curNode->lamp = std::make_unique<PointLamp>();
	scene.root->children.emplace_back(std::move(curNode));

	/*curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(-10, 11, -15).append(Transformation::scale(0.1, 0.1, 0.1));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*
	Transformation t3 = 
	Transformation t4 = Transformation::translate(4, 4, -12).append(Transformation::scale(4, 4, 4));
	Transformation t5 = Transformation::translate(-4, 4, -12).append(Transformation::scale(4, 4, 4));
	*/

	SceneNode& cameraNode = *(scene.root->children.emplace_back(std::make_unique<SceneNode>()));
	cameraNode.camera = std::make_unique<PerspectiveCamera>(width, height, origin, destination, lookup, fov);
	auto& camera = *(cameraNode.camera);

	//////

	FrameBuffer buffer(width, height);

	// subdivide the buffer in equal sized tiles
	for (auto tile : buffer.subdivide(64, 64)) {
		for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
			for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
				// create a ray through the center of the pixel.
				Ray ray = camera.generateRay(Sample(x + 0.5, y + 0.5));

				// test the scene on intersections
				auto hit = scene.traceRay(ray);

				// add a color contribution to the pixel
				if(hit.has_value())
				{
					buffer.setPixel(x, y, hit->second.get().material->getColorFor(hit->first, scene, 0));
				}
			}
		}
	}

	write_to_ppm(buffer, filename);
}

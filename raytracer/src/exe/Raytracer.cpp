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
#include "renderer/Renderer.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "io/tiny_obj_loader.h"
#include "shape/Box.h"
#include "material/NormalMaterial.h"
#include <iostream>
#include "material/TexCoordMaterial.h"
#include "io/PNGWriter.h"
#include "io/EXRWriter.h"

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
			texCoords.emplace_back(attrib.texcoords[i], 1-attrib.texcoords[i + 1]);
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

void render(std::string filename)
{
	int width = 1000;
	int height = 1000;
	double sensitivity = 1.0;
	double gamma = 2.2;
	bool quiet = false;
	Point origin(0, 0, 0);
	Point destination(0, 0, 1);
	Vector3 lookup(0, 1, 0);
	double fov = 60;

	//////

	auto normalMat = std::make_shared<NormalMaterial>();
	auto texCoordMat = std::make_shared<TexCoordMaterial>();

	auto diffuseMat = std::make_shared<DiffuseMaterial>();
	diffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	diffuseMat->ambientIntensity = 0.1;
	diffuseMat->diffuseColor = RGB{ 1.0, 1.0, 0.8 };
	diffuseMat->diffuseIntensity = 1.0;

	auto redDiffuseMat = std::make_shared<DiffuseMaterial>();
	redDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	redDiffuseMat->ambientIntensity = 0.1;
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

	Model bunnyModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/bunny_low.obj")), redDiffuseMat);
	Model triangleModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/triangle.obj")), whiteMat);

	//Model dragonModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/dragon_high.obj")), normalMat);

	//Model lucyModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/lucy.obj")), redDiffuseMat);

	//Model asianDragonModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/xyzrgb_dragon.obj")), redDiffuseMat);


	/*auto amDiffuseMat = std::make_shared<DiffuseMaterial>();
	amDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	amDiffuseMat->ambientIntensity = 0.7;
	amDiffuseMat->albedo = std::make_shared<Texture>(Texture::load("F:/models/autumn_casualwoman/autumn_casualwoman_02_-diffuse.png"));
	amDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	amDiffuseMat->diffuseIntensity = 1.0;
	Model amModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/autumn_casualwoman/autumn_casualwoman_02_highpoly.OBJ")), amDiffuseMat);*/

	/*auto coffeeDiffuseMat = std::make_shared<DiffuseMaterial>();
	coffeeDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	coffeeDiffuseMat->ambientIntensity = 0.7;
	coffeeDiffuseMat->albedoMap = std::make_shared<Texture>(Texture::load("F:/models/coffee/ark_coffee.png"));
	coffeeDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	coffeeDiffuseMat->diffuseIntensity = 1.0;
	Model coffeeModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/coffee/ARK_COFFEE_CUP.obj")), coffeeDiffuseMat);*/

	/*auto appleDiffuseMat = std::make_shared<DiffuseMaterial>();
	appleDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	appleDiffuseMat->ambientIntensity = 0.7;
	appleDiffuseMat->albedoMap = std::make_shared<Texture>(Texture::load("F:/models/apple/apple_texture.png"));
	appleDiffuseMat->normalMap = std::make_shared<Texture>(Texture::load("F:/models/apple/apple_normal.png"));
	appleDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	appleDiffuseMat->diffuseIntensity = 1.0;
	Model appleModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/apple/apple.obj")), appleDiffuseMat);*/

	/*auto handDiffuseMat = std::make_shared<DiffuseMaterial>();
	handDiffuseMat->ambientColor = RGB{ 0.7, 0.7, 1.0 };
	handDiffuseMat->ambientIntensity = 0.7;
	handDiffuseMat->albedoMap = std::make_shared<Texture>(Texture::load("F:/models/hand/HAND_C.png"));
	handDiffuseMat->normalMap = std::make_shared<Texture>(Texture::load("F:/models/hand/HAND_N.png"));
	handDiffuseMat->diffuseColor = RGB{ 1.0, 0.2, 0.2 };
	handDiffuseMat->diffuseIntensity = 1.0;
	Model handModel(std::make_shared<TriangleMesh>(loadMesh("F:/models/hand/uneedahand.obj")), handDiffuseMat);*/

	//Model teapotModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/teapot.obj")), normalMat);

	Model planeModel(std::make_shared<Plane>(), diffuseMat);

	//Model boxModel(std::make_shared<Box>(Vector3(0, 0, 0), Vector3(1, 1, 1)), redDiffuseMat);

	//Model planeMeshModel(std::make_shared<TriangleMesh>(loadMesh("F:\models/plane.obj")), redDiffuseMat);


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
	curNode->transform = Transformation::translate(0, -5, 10).append(Transformation::rotateY(90)).append(Transformation::scale(4, 4, 4));
	curNode->model = std::make_unique<Model>(appleModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*auto curNode = std::make_unique<DynamicSceneNode>();
	curNode->transform = Transformation::translate(0, -7, 15).append(Transformation::rotateY(90)).append(Transformation::scale(60, 60, 60));
	curNode->model = std::make_unique<Model>(handModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	/*{
		auto curNode = std::make_unique<DynamicSceneNode>();
		curNode->transform = Transformation::translate(-2, -4, 10).append(Transformation::rotateY(0)).append(Transformation::scale(4, 4, 4));
		curNode->model = std::make_unique<Model>(bunnyModel);
		scene.root->children.emplace_back(std::move(curNode));
	}*/
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
		curNode->transform = Transformation::translate(0, -8.2, 0);
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
		curNode->transform = Transformation::translate(7, -8.2, 35).append(Transformation::rotateY(225)).append(Transformation::scale(20, 20, 20));
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

	DynamicSceneNode& cameraNode = *(scene.root->children.emplace_back(std::make_unique<DynamicSceneNode>()));
	cameraNode.camera = std::make_unique<PerspectiveCamera>(width, height, origin, destination, lookup, fov);
	auto& camera = *(cameraNode.camera);

	//////

	std::cout << "Building scene." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	auto renderableScene = scene.build();
	auto finish = std::chrono::high_resolution_clock::now();
	double duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
	std::cout << "Scene build done in " << duration << " milliseconds." << std::endl;

	FrameBuffer buffer(width, height);

	start = std::chrono::high_resolution_clock::now();

	std::cout << "Rendering..." << std::endl;

	RenderSettings settings;
	settings.aaLevel = 1;
	render(renderableScene, buffer, settings);

	finish = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() / 1000.0;
	std::cout << "Rendering time: " << duration << " milliseconds" << std::endl;

	write_to_png_file(buffer, filename, -2.5f, 2.2);
	//write_to_exr_file(buffer, filename);
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
	//render("C:/Users/Wouter/Desktop/render/render.exr");
	render("C:/Users/Wouter/Desktop/render/render.png");
	//render("C:/Users/Wouter/Desktop/render/sphere.ppm");
}

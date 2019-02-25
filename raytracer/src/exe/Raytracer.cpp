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


int main(char** argc, int argv)
{
	int width = 1600;
	int height = 1600;
	double sensitivity = 1.0;
	double gamma = 2.2;
	bool quiet = false;
	Point origin(0, 0, 0);
	Point destination(0, 0, -1);
	Vector3 lookup(0, 1, 0);
	double fov = 90;
	std::string filename("C:/Users/Wouter/Desktop/img.ppm");

	//////

	Scene scene{};

	auto diffuseMat = std::make_shared<DiffuseMaterial>();
	diffuseMat->ambientColor = RGB{ 0.6, 0.6, 1.0 };
	diffuseMat->ambientIntensity = 0.3;
	diffuseMat->diffuseColor = RGB{ 1.0, 1.0, 0.8 };
	diffuseMat->diffuseIntensity = 1.0;

	Model sphereModel{};
	sphereModel.shape = std::make_shared<Sphere>();
	sphereModel.material = diffuseMat;

	Model planeModel{};
	planeModel.shape = std::make_shared<Plane>();
	planeModel.material = diffuseMat;
		
	auto curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(5, 3, -20).append(Transformation::scale(5, 5, 5));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));
	/*
	curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(4, 4, -22).append(Transformation::scale(4, 4, 4)).append(Transformation::rotateY(180));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));

	curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(0, 6, -18).append(Transformation::scale(2, 2, 2));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));*/

	curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(0, -5, 0);
	curNode->model = std::make_unique<Model>(planeModel);
	scene.root->children.emplace_back(std::move(curNode));

	curNode = std::make_unique<SceneNode>();
	//curNode->transform = Transformation::translate(-10, 10, -15);
	curNode->transform = Transformation::translate(0, 12, -20);
	curNode->lamp = std::make_unique<PointLamp>();
	scene.root->children.emplace_back(std::move(curNode));

	curNode = std::make_unique<SceneNode>();
	curNode->transform = Transformation::translate(-10, 11, -15).append(Transformation::scale(0.1, 0.1, 0.1));
	curNode->model = std::make_unique<Model>(sphereModel);
	scene.root->children.emplace_back(std::move(curNode));

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

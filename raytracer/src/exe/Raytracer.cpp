#include "raytracer.h"
#include <math/ray.h>
#include "film/FrameBuffer.h"
#include "io/PPMFile.h"
#include "math/Transformation.h"
#include "shape/Sphere.h"
#include "camera/PerspectiveCamera.h"

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

	Transformation t1 = Transformation::translate(0, 0, -10).append(Transformation::scale(5, 5, 5));
	Transformation t2 = Transformation::translate(4, -4, -12).append(Transformation::scale(4, 4, 4));
	Transformation t3 = Transformation::translate(-4, -4, -12).append(Transformation::scale(4, 4, 4));
	Transformation t4 = Transformation::translate(4, 4, -12).append(Transformation::scale(4, 4, 4));
	Transformation t5 = Transformation::translate(-4, 4, -12).append(Transformation::scale(4, 4, 4));
	std::vector<Sphere> shapes;
	shapes.emplace_back<Sphere>(t1);
	shapes.emplace_back<Sphere>(t2);
	shapes.emplace_back<Sphere>(t3);
	shapes.emplace_back<Sphere>(t4);
	shapes.emplace_back<Sphere>(t5);

	PerspectiveCamera camera(width, height, origin, destination, lookup, fov);
	FrameBuffer buffer(width, height);

	// subdivide the buffer in equal sized tiles
	for (auto tile : buffer.subdivide(64, 64)) {
		for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
			for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
				// create a ray through the center of the
				// pixel.
				Ray ray = camera.generateRay(Sample(x + 0.5, y + 0.5));

				// test the scene on intersections
				bool hit = false;
				for (auto shape : shapes) {
					if (shape.intersect(ray)) {
						hit = true;
						break;
					}
				}

				// add a color contribution to the pixel
				if (hit)
					buffer.setPixel(x, y, RGB{ 1.0, 0.0, 0.0 });
			}
		}
	}

	write_to_ppm(buffer, filename);
}

#include "raytracer.h"
#include "math/ray.h"
#include "film/FrameBuffer.h"
#include "io/PPMFile.h"

int main(char** argc, int argv)
{
	int width = 640;
	int height = 640;
	double sensitivity = 1.0;
	double gamma = 2.2;
	bool gui = true;
	bool quiet = false;
	Point origin(0, 0, 0);
	Point destination(0, 0, -1);
	Vector3 lookup(0, 1, 0);
	double fov = 90;
	std::string filename("output.png");

	FrameBuffer buffer(256, 512);
	buffer.setPixel(5, 10, RGB{ 1.0, 0.0, 0.0 });
	write_to_ppm(buffer, "C:/Users/Wouter/Desktop/img.ppm");
}

#include "EXRWriter.h"
#include "film/FrameBuffer.h"
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

void write_to_exr_file(const FrameBuffer& buffer, std::string filepath)
{
	EXRHeader header;
	InitEXRHeader(&header);

	EXRImage image;
	InitEXRImage(&image);

	image.num_channels = 3;

	auto width = buffer.getHorizontalResolution();
	auto height = buffer.getVerticalResolution();
	std::vector<float> images[3];
	images[0].resize(width * height);
	images[1].resize(width * height);
	images[2].resize(width * height);

	for(auto y = 0; y < height; y++)
	{
		for(auto x = 0; x < width; x++)
		{
			auto pixel = buffer.getPixel(x, y);
			auto i = ((y * width) + x);
			images[0][i] = pixel.getRed();
			images[1][i] = pixel.getGreen();
			images[2][i] = pixel.getBlue();
		}
	}

	float* image_ptr[3];
	image_ptr[0] = &(images[2].at(0)); // B
	image_ptr[1] = &(images[1].at(0)); // G
	image_ptr[2] = &(images[0].at(0)); // R

	image.images = (unsigned char**)image_ptr;
	image.width = width;
	image.height = height;

	header.num_channels = 3;
	header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);
	// Must be BGR(A) order, since most of EXR viewers expect this channel order.
	strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
	strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
	strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

	header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
	header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
	for (int i = 0; i < header.num_channels; i++) {
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
	}

	std::string error;
	const char* err;
	int ret = SaveEXRImageToFile(&image, &header, filepath.c_str(), &err);
	if (ret != TINYEXR_SUCCESS) {
		error = err;
	}

	free(header.channels);
	free(header.pixel_types);
	free(header.requested_pixel_types);

	if (ret != TINYEXR_SUCCESS) {
		throw std::runtime_error(error);
	}
}

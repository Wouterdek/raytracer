#include "Renderer.h"
#include <thread>
#include <random>
//#include <execution>
#include "tbb/tbb.h"
#include "math/Ray.h"
#include "camera/ICamera.h"
#include "ProgressMonitor.h"

#undef min

class AASampleGenerator
{
public:
	AASampleGenerator(int aaLevel)
		: dist(0, 1.0 / aaLevel), aaLevel(aaLevel)
	{}

	Sample generateSample(std::random_device& rd, int pixelX, int pixelY, int sampleI)
	{
		if(aaLevel == 1)
		{
			return Sample(pixelX + 0.5, pixelY + 0.5);
		}else
		{
			const auto x = pixelX + (static_cast<double>(sampleI % aaLevel) / aaLevel) + dist(rd);
			const auto y = pixelY + (static_cast<double>(std::floor(sampleI / aaLevel)) / aaLevel) + dist(rd);
			return Sample(x, y);
		}
	}
private:
	std::uniform_real_distribution<double> dist;
	int aaLevel;
};

thread_local std::random_device rd;

void render(const Scene& scene, FrameBuffer& buffer, const Tile& tile, const RenderSettings& renderSettings, ProgressMonitor progressMon, bool multithreaded)
{
	std::vector<Tile> tiles;

	if(multithreaded)
	{
		const int cpuCount = std::thread::hardware_concurrency();
		const auto taskMultiplier = 3; // tasks per cpu, to compensate for slow vs fast tasks

		const auto taskCount = std::min({ cpuCount * taskMultiplier, tile.getWidth(), tile.getHeight() });

		const auto width = tile.getWidth() / taskCount;
		const auto height = tile.getHeight() / taskCount;

		tiles = tile.subdivide(width, height);
	}
	else
	{
		tiles.push_back(tile);
	}

	ProgressTracker progress(progressMon, tiles.size());

	const ICamera* mainCam = nullptr;
	for(const auto& camera : scene.getCameras()){
        if(camera.getData().isMainCamera){
            mainCam = &camera.getData();
        }
	}
	if(mainCam == nullptr){
	    if(!scene.getCameras().empty()){
	        mainCam = &scene.getCameras()[0].getData();
	    }else{
            throw std::runtime_error("No camera in scene.");
	    }
	}

	//std::for_each(std::execution::par_unseq, tiles.begin(), tiles.end(), [&scene, &buffer, &renderSettings, &camera](const Tile& curTile)
    tbb::parallel_for_each(tiles.begin(), tiles.end(), [&scene, &buffer, &renderSettings, &progress, &camera = *mainCam](const Tile& curTile)
	{
		AASampleGenerator aa(renderSettings.aaLevel);

		for (int y = curTile.getYStart(); y < curTile.getYEnd(); ++y) {
			for (int x = curTile.getXStart(); x < curTile.getXEnd(); ++x) {
				RGB pixelValue{};

				for(int i = 0; i < renderSettings.aaLevel; i++)
				{
					// create a ray through the center of the pixel.
					Ray ray = camera.generateRay(aa.generateSample(rd, x, y, i), buffer.getHorizontalResolution(), buffer.getVerticalResolution());

					// test the scene on intersections
					//auto start = std::chrono::high_resolution_clock::now();

					auto hit = scene.traceRay(ray);

					//auto finish = std::chrono::high_resolution_clock::now();
					//double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count()/1000.0;

					//buffer.setPixel(x, y, RGB(duration));
					//maxVal = std::max(maxVal, duration);

					/*RGB bvhMarker(0, BVHDiag::Levels, 0);
					pixelValue = pixelValue.add(bvhMarker);
					BVHDiag::Levels = 0;*/

					// add a color contribution to the pixel
					if (hit.has_value())
					{
						/*auto hitpoint = hit->getGeometryInfo().getHitpoint();
						auto depth = std::log(hitpoint.norm())/4.0f;
						depth = std::clamp<float>(depth, 0, 1);
						buffer.setPixel(x, y, RGB(depth));*/

						pixelValue = pixelValue.add(hit->getModelNode().getData().getMaterial().getColorFor(*hit, scene, 0));
					}
				}
				
				buffer.setPixel(x, y, pixelValue.divide(renderSettings.aaLevel));
			}

			//std::cout << y * 100 / curTile.getYEnd() << "% done\r";
		}
        progress.signalJobFinished();
	});
}

void render(const Scene& scene, FrameBuffer& buffer, const RenderSettings& renderSettings, ProgressMonitor progressMon)
{
	render(scene, buffer, Tile(0, 0, buffer.getHorizontalResolution(), buffer.getVerticalResolution()), renderSettings, progressMon);
}
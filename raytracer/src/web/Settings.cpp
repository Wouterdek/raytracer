#include <emscripten/emscripten.h>

#include "Settings.h"
#include "api.h"

API_EXPORT Settings* newSettings()
{
    return new Settings{};
}

API_EXPORT void freeSettings(Settings* settings)
{
    delete settings;
}

API_EXPORT void setWorkingDirectory(Settings* settings, const char* path)
{
    settings->workDir = std::string(path);
}

API_EXPORT void setSceneFile(Settings* settings, const char* path)
{
    settings->sceneFile = std::string(path);
}

API_EXPORT void setImageDimensions(Settings* settings, int width, int height, int xStart, int yStart, int xEnd, int yEnd)
{
    settings->width = width;
    settings->height = height;
    settings->xStart = xStart;
    settings->yStart = yStart;
    settings->xEnd = xEnd;
    settings->yEnd = yEnd;
}

API_EXPORT void setImageLDRSettings(Settings* settings, double exposure, double gamma)
{
    settings->exposure = exposure;
    settings->gamma = gamma;
}

API_EXPORT void setAntiAliasing(Settings* settings, int geometryAAModifier, int materialAAModifier)
{
    settings->geometryAAModifier = geometryAAModifier;
    settings->materialAAModifier = materialAAModifier;
}

API_EXPORT void setOutputFile(Settings* settings, const char* filetypePtr, const char* path)
{
    std::string filetype(filetypePtr);
    if(filetype != "exr" && filetype != "png" && filetype != "ppm" && filetype != "tile")
    {
        throw std::runtime_error("Invalid output type: " + filetype);
    }

    settings->outputType = filetype;
    settings->outputPath = std::string(path);
}

API_EXPORT void setPhotonMapFile(Settings* settings, const char* file, bool load, bool save)
{
    settings->photonMapFile = std::string(file);
    settings->loadPhotonMapFromFile = load;
    settings->savePhotonMapToFile = save;
}

API_EXPORT void setPhotonMapMode(Settings* settings, const char* mode)
{
    PhotonMapMode photonMappingMode = PhotonMapMode::none;
    std::string pmmodeString(mode);
    if(pmmodeString == "caustics")
    {
        photonMappingMode = PhotonMapMode::caustics;
    }
    else if(pmmodeString == "full")
    {
        photonMappingMode = PhotonMapMode::full;
    }
    else if(pmmodeString == "none")
    {}
    else
    {
        throw std::runtime_error("Invalid photonmapping mode!");
    }
    settings->photonMappingMode = photonMappingMode;
}

API_EXPORT void setPhotonMapSampleDepth(Settings* settings, int depth)
{
    settings->photonMappingSampleDepth = depth;
}
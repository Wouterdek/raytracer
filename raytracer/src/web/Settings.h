#pragma once

#include <string>
#include <photonmapping/PhotonMap.h>

struct Settings
{
    std::string workDir = "/";

    int width = 400;
    int height = 300;
    int xStart = 0;
    int yStart = 0;
    int xEnd = 400;
    int yEnd = 300;

    int geometryAAModifier = 2;
    int materialAAModifier = 2;

    double exposure = -2.5;
    double gamma = 2.2;

    std::string outputType = "png";
    std::string outputPath = "/output.png";

    std::string sceneFile = "/scene.glb";

    bool loadPhotonMapFromFile = false;
    bool savePhotonMapToFile = false;
    std::string photonMapFile = "/photonmap";
    PhotonMapMode photonMappingMode;
    int photonMappingSampleDepth = 0;
};

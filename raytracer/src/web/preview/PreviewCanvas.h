#pragma once

#include <film/FrameBuffer.h>
#include <memory>
#include <thread>

class PreviewCanvas
{
public:
    explicit PreviewCanvas(std::shared_ptr<FrameBuffer> frameBuffer, const std::string& canvasId);
    void show();
    void showAsync();
    void stop();
    void wait();

private:
    std::shared_ptr<FrameBuffer> img;
    std::string canvasId;
    std::unique_ptr<std::thread> uiThread;
    bool stopped = false;
};

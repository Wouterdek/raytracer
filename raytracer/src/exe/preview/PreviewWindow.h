#pragma once

#include <film/FrameBuffer.h>
#include <memory>
#include <thread>

class PreviewWindow
{
public:
    explicit PreviewWindow(std::shared_ptr<FrameBuffer> frameBuffer);
    void show();
    void showAsync();
    void wait();

private:
    std::shared_ptr<FrameBuffer> img;
    std::unique_ptr<std::thread> uiThread;
};

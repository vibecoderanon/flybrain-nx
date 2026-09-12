#pragma once

#include "../simulation/lif_engine.hpp"
#include <cstdint>

namespace flybrain {

class TelemetryHUD {
public:
    TelemetryHUD();
    ~TelemetryHUD();

    /**
     * @brief Render the on-screen telemetry overlay and controller guide onto the framebuffer
     */
    void render(uint32_t* framebuffer, int width, int height, LIFEngine& engine, float fps);

private:
    // Built-in crisp bitmap font renderer
    static void drawChar(uint32_t* fb, int fb_w, int fb_h, int x, int y, char c, uint32_t color);
    static void drawString(uint32_t* fb, int fb_w, int fb_h, int x, int y, const char* str, uint32_t color);
    static void drawRectFilled(uint32_t* fb, int fb_w, int fb_h, int x, int y, int w, int h, uint32_t color);
    static void drawProgressBar(uint32_t* fb, int fb_w, int fb_h, int x, int y, int w, int h, float ratio, uint32_t fill_color);
};

} // namespace flybrain

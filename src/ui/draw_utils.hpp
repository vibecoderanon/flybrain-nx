#pragma once

#include <cstdint>

namespace flybrain {

class DrawUtils {
public:
    static void drawChar(uint32_t* fb, int fb_w, int fb_h, int x, int y, char c, uint32_t color, int scale = 2);
    static void drawString(uint32_t* fb, int fb_w, int fb_h, int x, int y, const char* str, uint32_t color, int scale = 2);
    static void drawRectFilled(uint32_t* fb, int fb_w, int fb_h, int x, int y, int w, int h, uint32_t color);
    static void drawRectOutline(uint32_t* fb, int fb_w, int fb_h, int x, int y, int w, int h, int thickness, uint32_t color);
    static void drawLine(uint32_t* fb, int fb_w, int fb_h, int x0, int y0, int x1, int y1, uint32_t color);
    static void drawCircleFilled(uint32_t* fb, int fb_w, int fb_h, int cx, int cy, int radius, uint32_t color);
};

} // namespace flybrain

#pragma once

#include "../simulation/lif_engine.hpp"
#include <cstdint>

namespace flybrain {

class TelemetryHUD {
public:
    TelemetryHUD();
    ~TelemetryHUD();

    /**
     * @brief Render the bottom telemetry bar and controller guide onto the framebuffer
     */
    void render(uint32_t* framebuffer, int width, int height, LIFEngine& engine, float fps, bool show_axon_lines = true);
};

} // namespace flybrain

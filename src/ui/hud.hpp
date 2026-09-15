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

    /**
     * @brief Render spectator neural gauges (Stonkfly / Doomfly inspired) onto top right panel
     */
    void renderSpectatorGauges(uint32_t* fb, int width, int height, const LIFEngine& engine, int pace_mode);
};

} // namespace flybrain

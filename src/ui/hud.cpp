#include "hud.hpp"
#include "draw_utils.hpp"
#include <cstdio>

namespace flybrain {

TelemetryHUD::TelemetryHUD() = default;
TelemetryHUD::~TelemetryHUD() = default;

void TelemetryHUD::render(uint32_t* fb, int width, int height, LIFEngine& engine, float fps, bool show_axon_lines) {
    if (!fb || width <= 0 || height <= 0) return;

    int bar_y = height - 40;
    int bar_h = 40;

    // 1. Draw Background Bar
    DrawUtils::drawRectFilled(fb, width, height, 0, bar_y, width, bar_h, 0xFF080C14);
    DrawUtils::drawLine(fb, width, height, 0, bar_y, width - 1, bar_y, 0xFF1E293B);

    // 2. Query Engine Telemetry
    auto tel = engine.getTelemetry();
    float spikes_k = static_cast<float>(tel.total_spikes_recent) / 1000.0f;

    // 3. Top Row Telemetry
    char tel_buf[128];
    std::snprintf(tel_buf, sizeof(tel_buf), "SIM: %.0f Hz | SPIKES: %.1fk/s | RENDER: %.0f FPS | AXONS: %s",
                  tel.simulation_hz, spikes_k, fps, show_axon_lines ? "ON" : "OFF");
    DrawUtils::drawString(fb, width, height, 16, bar_y + 6, tel_buf, 0xFF38BDF8, 1);

    const char* controls_right = "[D-Pad/LS] Move  [A] Fill  [B] Cross  [X] Autopilot  [Y] Axons  [L/R] Puzzle";
    DrawUtils::drawString(fb, width, height, 640, bar_y + 6, controls_right, 0xFFF1F5F9, 1);

    // 4. Bottom Row Details
    const char* bio_info = "CONNECTOME: Drosophila melanogaster SNN | Attractor CSP Solver";
    DrawUtils::drawString(fb, width, height, 16, bar_y + 22, bio_info, 0xFF64748B, 1);

    const char* sub_controls = "[RS] Orbit 3D Brain Camera   |   (+) Exit to HOME Menu";
    DrawUtils::drawString(fb, width, height, 760, bar_y + 22, sub_controls, 0xFF94A3B8, 1);
}

} // namespace flybrain

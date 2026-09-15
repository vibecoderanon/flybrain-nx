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

    const char* controls_right = "[D-Pad] Move  [A] Mark/Step  [B] Cross  [X] Mode  [Y] Axons  [L/R] Puzzles";
    DrawUtils::drawString(fb, width, height, 640, bar_y + 6, controls_right, 0xFFF1F5F9, 1);

    // 4. Bottom Row Details
    const char* bio_info = "CONNECTOME: Drosophila melanogaster SNN | Attractor CSP Solver";
    DrawUtils::drawString(fb, width, height, 16, bar_y + 22, bio_info, 0xFF64748B, 1);

    const char* sub_controls = "[RS] Orbit 3D Brain Camera   |   (+) Exit to HOME Menu";
    DrawUtils::drawString(fb, width, height, 760, bar_y + 22, sub_controls, 0xFF94A3B8, 1);
}

void TelemetryHUD::renderSpectatorGauges(uint32_t* fb, int width, int height, const LIFEngine& engine, int pace_mode) {
    if (!fb || width <= 0 || height <= 0) return;

    int gx = 648;
    int gy = 8;
    int gw = 620;
    int gh = 58;

    // Outer container card
    DrawUtils::drawRectFilled(fb, width, height, gx, gy, gw, gh, 0xEE0B1220);
    DrawUtils::drawRectOutline(fb, width, height, gx, gy, gw, gh, 1, 0xFF1E293B);

    // Header title
    DrawUtils::drawString(fb, width, height, gx + 12, gy + 6, "SPECTATOR NEURAL GAUGES (MaleCNS v1.0)", 0xFF94A3B8, 1);

    // Mode badge
    const char* mode_str = "[PACE: OBSERVE 2.2s]";
    uint32_t mode_col = 0xFF38BDF8; // Cyan
    if (pace_mode == 1) {
        mode_str = "[PACE: FAST 0.9s]";
        mode_col = 0xFFFBBF24; // Amber
    } else if (pace_mode == 2) {
        mode_str = "[PACE: STEP (PRESS A)]";
        mode_col = 0xFFA855F7; // Purple
    } else if (pace_mode == 3) {
        mode_str = "[MANUAL JOY-CON]";
        mode_col = 0xFF4ADE80; // Green
    }
    DrawUtils::drawString(fb, width, height, gx + 440, gy + 6, mode_str, mode_col, 1);

    // 4 Live Gauges
    int bar_w = 142;
    int bar_h = 13;
    int bar_gap = 8;
    int start_x = gx + 12;
    int label_y = gy + 22;
    int bar_y = gy + 36;

    // Helper lambda to draw a single gauge
    auto drawGauge = [&](int index, const char* label, float value, uint32_t fill_col, uint32_t glow_col) {
        int bx = start_x + index * (bar_w + bar_gap);

        // Label
        DrawUtils::drawString(fb, width, height, bx, label_y, label, fill_col, 1);

        // Background groove
        DrawUtils::drawRectFilled(fb, width, height, bx, bar_y, bar_w, bar_h, 0xFF111827);
        DrawUtils::drawRectOutline(fb, width, height, bx, bar_y, bar_w, bar_h, 1, 0xFF374151);

        // Filled level
        float val_clamped = std::clamp(value, 0.0f, 1.0f);
        int fill_pixels = static_cast<int>(val_clamped * static_cast<float>(bar_w - 2));
        if (fill_pixels > 0) {
            uint32_t cur_col = (val_clamped > 0.65f) ? glow_col : fill_col;
            DrawUtils::drawRectFilled(fb, width, height, bx + 1, bar_y + 1, fill_pixels, bar_h - 2, cur_col);
        }
    };

    // 1. PAM11 Dopamine (Emerald Green)
    drawGauge(0, "PAM11 REWARD", engine.getDopamineLevel(), 0xFF10B981, 0xFF34D399);

    // 2. PPL101 Aversive (Crimson Red)
    drawGauge(1, "PPL101 AVERSIVE", engine.getAversiveLevel(), 0xFFEF4444, 0xFFF87171);

    // 3. Optic Scan (Cyan)
    drawGauge(2, "OPTIC SCAN", engine.getOpticScanLevel(), 0xFF06B6D4, 0xFF38BDF8);

    // 4. Compass CX (Gold)
    drawGauge(3, "COMPASS (CX)", engine.getCompassLevel(), 0xFFF59E0B, 0xFFFBBF24);
}

void TelemetryHUD::renderCircuitHighway(uint32_t* fb, int width, int height, int cognitive_phase, float anim_time) {
    if (!fb || width <= 0 || height <= 0) return;

    int gx = 648;
    int gy = 68;
    int gw = 620;
    int gh = 24;

    // Outer container
    DrawUtils::drawRectFilled(fb, width, height, gx, gy, gw, gh, 0xEE0B1220);
    DrawUtils::drawRectOutline(fb, width, height, gx, gy, gw, gh, 1, 0xFF1E293B);

    struct Stage {
        const char* label;
        int phase_id;
        uint32_t base_col;
        uint32_t glow_col;
    };

    Stage stages[4] = {
        {"1:OPTIC", 1, 0xFF06B6D4, 0xFF38BDF8},
        {"2:COMPASS", 2, 0xFFF59E0B, 0xFFFDE047},
        {"3:MUSHROOM", 3, 0xFFF43F5E, 0xFFFDA4AF},
        {"4:MOTOR", 4, 0xFFF97316, 0xFFFDBA74}
    };

    int stage_w = 114;
    int stage_h = 16;
    int start_x = gx + 8;
    int stage_y = gy + 4;
    int arrow_gap = 40;

    for (int i = 0; i < 4; ++i) {
        int sx = start_x + i * (stage_w + arrow_gap);
        bool is_active = (cognitive_phase == stages[i].phase_id);

        uint32_t card_bg = is_active ? 0xFF1E293B : 0xFF0F172A;
        uint32_t border_col = is_active ? 0xFFFFFFFF : stages[i].base_col;
        uint32_t text_col = is_active ? 0xFFFFFFFF : stages[i].glow_col;

        DrawUtils::drawRectFilled(fb, width, height, sx, stage_y, stage_w, stage_h, card_bg);
        DrawUtils::drawRectOutline(fb, width, height, sx, stage_y, stage_w, stage_h, 1, border_col);

        // Status indicator dot
        uint32_t dot_col = is_active ? 0xFF22C55E : stages[i].base_col;
        DrawUtils::drawCircleFilled(fb, width, height, sx + 8, stage_y + 8, 3, dot_col);

        DrawUtils::drawString(fb, width, height, sx + 16, stage_y + 4, stages[i].label, text_col, 1);

        // Connector Arrow between stages
        if (i < 3) {
            int ax = sx + stage_w + 6;
            int ay = stage_y + 8;
            uint32_t arrow_col = (cognitive_phase > stages[i].phase_id) ? 0xFF38BDF8 : 0xFF475569;
            if (is_active) {
                float p = 0.5f + 0.5f * std::sin(anim_time * 12.0f);
                arrow_col = (p > 0.4f) ? 0xFFFFFFFF : 0xFF38BDF8;
            }
            DrawUtils::drawLine(fb, width, height, ax, ay, ax + 18, ay, arrow_col);
            DrawUtils::drawLine(fb, width, height, ax + 14, ay - 3, ax + 18, ay, arrow_col);
            DrawUtils::drawLine(fb, width, height, ax + 14, ay + 3, ax + 18, ay, arrow_col);
        }
    }
}

} // namespace flybrain

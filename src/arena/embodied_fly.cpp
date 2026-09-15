#include "embodied_fly.hpp"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace flybrain {

EmbodiedFlyArena::EmbodiedFlyArena() = default;
EmbodiedFlyArena::~EmbodiedFlyArena() = default;

void EmbodiedFlyArena::init() {
    m_fly.x = 240.0f;
    m_fly.y = 360.0f;
    m_fly.heading = 0.0f;
    m_fly.current_r = 0;
    m_fly.current_c = 0;
    m_fly.target_r = 0;
    m_fly.target_c = 0;
    m_fly.scan_type = -1;
    m_fly.scan_idx = -1;
    m_fly.state = FlyActionState::Idle;
    m_fly.state_timer = 0.0f;
    m_fly.wing_phase = 0.0f;
    m_fly.proboscis_ext = 0.0f;
    m_fly.pending_action = CellState::Unknown;
    m_fly.contact_triggered = false;
    std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text), "Drosophila connectome initialized. Attractor ready.");
}

void EmbodiedFlyArena::reset(float start_x, float start_y) {
    m_fly.x = start_x;
    m_fly.y = start_y;
    m_fly.heading = 0.0f;
    m_fly.current_r = 0;
    m_fly.current_c = 0;
    m_fly.target_r = 0;
    m_fly.target_c = 0;
    m_fly.scan_type = -1;
    m_fly.scan_idx = -1;
    m_fly.state = FlyActionState::Idle;
    m_fly.state_timer = 0.0f;
    m_fly.proboscis_ext = 0.0f;
    m_fly.pending_action = CellState::Unknown;
    m_fly.contact_triggered = false;
    std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text), "New puzzle loaded. Ready for sensory scan.");
}

void EmbodiedFlyArena::startDeliberation(int r, int c, CellState action, int scan_type, int scan_idx,
                                         const PicrossBoard& board,
                                         float tile_size, float grid_origin_x, float grid_origin_y,
                                         float speed_mult) {
    (void)board;
    m_fly.target_r = r;
    m_fly.target_c = c;
    m_fly.pending_action = action;
    m_fly.scan_type = scan_type;
    m_fly.scan_idx = scan_idx;
    m_fly.contact_triggered = false;
    m_fly.proboscis_ext = 0.0f;

    // Stage 1: Visual sensory scan
    m_fly.state = FlyActionState::ScanningLine;
    m_fly.state_timer = 0.50f / std::max(0.1f, speed_mult);

    // Aim heading toward clues or target tile
    float tx = grid_origin_x + (static_cast<float>(c) + 0.5f) * tile_size;
    float ty = grid_origin_y + (static_cast<float>(r) + 0.5f) * tile_size;
    float dx = tx - m_fly.x;
    float dy = ty - m_fly.y;
    if (dx * dx + dy * dy > 1.0f) {
        m_fly.heading = std::atan2(dy, dx);
    }

    if (scan_type == 0) {
        std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                      "Sensory Scan: Inspecting Row %d constraints...", scan_idx + 1);
    } else if (scan_type == 1) {
        std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                      "Sensory Scan: Inspecting Col %d constraints...", scan_idx + 1);
    } else {
        std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                      "Sensory Scan: Evaluating global attractor constraints...");
    }
}

void EmbodiedFlyArena::commandMove(int r, int c, CellState action, float tile_size, float grid_origin_x, float grid_origin_y) {
    m_fly.target_r = r;
    m_fly.target_c = c;
    m_fly.pending_action = action;
    m_fly.state = FlyActionState::Walking;
    m_fly.contact_triggered = false;
    m_fly.proboscis_ext = 0.0f;

    float tx = grid_origin_x + (static_cast<float>(c) + 0.5f) * tile_size;
    float ty = grid_origin_y + (static_cast<float>(r) + 0.5f) * tile_size;
    float dx = tx - m_fly.x;
    float dy = ty - m_fly.y;
    if (dx * dx + dy * dy > 1.0f) {
        m_fly.heading = std::atan2(dy, dx);
    }

    std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                  "Manual Move: Crawling to tile (%d, %d)...", r, c);
}

void EmbodiedFlyArena::triggerVictory() {
    m_fly.state = FlyActionState::Victory;
    std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                  "PUZZLE SOLVED! Connectome Attractor Converged!");
}

void EmbodiedFlyArena::update(LIFEngine& engine, float dt_sec, float tile_size, float grid_origin_x, float grid_origin_y, float speed_mult) {
    m_fly.contact_triggered = false;
    m_fly.wing_phase += dt_sec * 35.0f * speed_mult;

    switch (m_fly.state) {
        case FlyActionState::ScanningLine: {
            m_fly.state_timer -= dt_sec;
            engine.setOpticScanActive(true);

            // Subtle body wobble during sensory scan
            m_fly.heading += std::sin(m_fly.wing_phase * 0.4f) * dt_sec * 1.5f;

            if (m_fly.state_timer <= 0.0f) {
                // Transition to Stage 2: Walking / Navigation
                m_fly.state = FlyActionState::Walking;
                engine.setOpticScanActive(false);
                engine.setCompassActive(true);

                float tx = grid_origin_x + (static_cast<float>(m_fly.target_c) + 0.5f) * tile_size;
                float ty = grid_origin_y + (static_cast<float>(m_fly.target_r) + 0.5f) * tile_size;
                float dx = tx - m_fly.x;
                float dy = ty - m_fly.y;
                if (dx * dx + dy * dy > 1.0f) {
                    m_fly.heading = std::atan2(dy, dx);
                }

                std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                              "Navigation (CX): Crawling toward tile (%d, %d)...",
                              m_fly.target_r, m_fly.target_c);
            }
            break;
        }

        case FlyActionState::Walking: {
            engine.setCompassActive(true);
            float tx = grid_origin_x + (static_cast<float>(m_fly.target_c) + 0.5f) * tile_size;
            float ty = grid_origin_y + (static_cast<float>(m_fly.target_r) + 0.5f) * tile_size;

            float dx = tx - m_fly.x;
            float dy = ty - m_fly.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist > 3.0f) {
                float target_heading = std::atan2(dy, dx);
                float angle_diff = target_heading - m_fly.heading;
                while (angle_diff > 3.14159f) angle_diff -= 6.28318f;
                while (angle_diff < -3.14159f) angle_diff += 6.28318f;
                m_fly.heading += angle_diff * std::min(1.0f, dt_sec * 14.0f * speed_mult);

                float move_step = std::min(dist, 260.0f * speed_mult * dt_sec);
                m_fly.x += std::cos(m_fly.heading) * move_step;
                m_fly.y += std::sin(m_fly.heading) * move_step;
            } else {
                // Arrived at tile: Transition to Stage 3: Inspecting
                m_fly.x = tx;
                m_fly.y = ty;
                m_fly.current_r = m_fly.target_r;
                m_fly.current_c = m_fly.target_c;
                engine.setCompassActive(false);

                m_fly.state = FlyActionState::Inspecting;
                m_fly.state_timer = 0.40f / std::max(0.1f, speed_mult);

                std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                              "Deliberation (MB): Verifying overlap at (%d, %d)...",
                              m_fly.current_r, m_fly.current_c);
            }
            break;
        }

        case FlyActionState::Inspecting: {
            m_fly.state_timer -= dt_sec;
            // Antennae and body twitch
            m_fly.heading += std::sin(m_fly.wing_phase * 0.8f) * dt_sec * 2.0f;

            if (m_fly.state_timer <= 0.0f) {
                // Transition to Stage 4: Synchronized Actuation
                m_fly.state = FlyActionState::Actuating;
                m_fly.state_timer = 0.35f / std::max(0.1f, speed_mult);
                m_fly.proboscis_ext = 0.0f;

                // TRIGGER PHYSICAL CONTACT ON THIS EXACT FRAME!
                m_fly.contact_triggered = true;

                if (m_fly.pending_action == CellState::Filled) {
                    std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                                  "Actuation: Proboscis inking tile (%d, %d)! (+PAM11)",
                                  m_fly.current_r, m_fly.current_c);
                } else {
                    std::snprintf(m_fly.thought_text, sizeof(m_fly.thought_text),
                                  "Actuation: Leg claw scratching cross (%d, %d)",
                                  m_fly.current_r, m_fly.current_c);
                }
            }
            break;
        }

        case FlyActionState::Actuating: {
            m_fly.state_timer -= dt_sec;
            m_fly.proboscis_ext = std::min(1.0f, m_fly.proboscis_ext + dt_sec * 6.0f * speed_mult);

            if (m_fly.state_timer <= 0.0f) {
                m_fly.state = FlyActionState::Idle;
                m_fly.proboscis_ext = 0.0f;
                m_fly.scan_type = -1;
                m_fly.scan_idx = -1;
                m_fly.pending_action = CellState::Unknown;
            }
            break;
        }

        case FlyActionState::Victory: {
            m_fly.heading += dt_sec * 8.0f;
            m_fly.wing_phase += dt_sec * 70.0f;
            break;
        }

        case FlyActionState::Idle:
        default:
            break;
    }
}

} // namespace flybrain

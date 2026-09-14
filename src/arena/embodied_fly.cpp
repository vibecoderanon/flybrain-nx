#include "embodied_fly.hpp"
#include <algorithm>

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
    m_fly.state = FlyActionState::Idle;
    m_fly.state_timer = 0.0f;
    m_fly.wing_phase = 0.0f;
    m_fly.pending_action = CellState::Unknown;
}

void EmbodiedFlyArena::reset(float start_x, float start_y) {
    m_fly.x = start_x;
    m_fly.y = start_y;
    m_fly.heading = 0.0f;
    m_fly.current_r = 0;
    m_fly.current_c = 0;
    m_fly.target_r = 0;
    m_fly.target_c = 0;
    m_fly.state = FlyActionState::Idle;
    m_fly.state_timer = 0.0f;
    m_fly.pending_action = CellState::Unknown;
}

void EmbodiedFlyArena::commandMove(int r, int c, CellState action, float tile_size, float grid_origin_x, float grid_origin_y) {
    m_fly.target_r = r;
    m_fly.target_c = c;
    m_fly.pending_action = action;
    m_fly.state = FlyActionState::Walking;

    // Calculate target heading immediately
    float tx = grid_origin_x + (static_cast<float>(c) + 0.5f) * tile_size;
    float ty = grid_origin_y + (static_cast<float>(r) + 0.5f) * tile_size;
    float dx = tx - m_fly.x;
    float dy = ty - m_fly.y;
    if (dx * dx + dy * dy > 1.0f) {
        m_fly.heading = std::atan2(dy, dx);
    }
}

void EmbodiedFlyArena::triggerVictory() {
    m_fly.state = FlyActionState::Victory;
}

void EmbodiedFlyArena::update(LIFEngine& engine, float dt_sec, float tile_size, float grid_origin_x, float grid_origin_y) {
    (void)engine; // Engine motor cues can modulate speed

    m_fly.wing_phase += dt_sec * 35.0f;

    switch (m_fly.state) {
        case FlyActionState::Walking: {
            float tx = grid_origin_x + (static_cast<float>(m_fly.target_c) + 0.5f) * tile_size;
            float ty = grid_origin_y + (static_cast<float>(m_fly.target_r) + 0.5f) * tile_size;

            float dx = tx - m_fly.x;
            float dy = ty - m_fly.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist > 3.0f) {
                // Smoothly steer heading toward target
                float target_heading = std::atan2(dy, dx);
                float angle_diff = target_heading - m_fly.heading;
                while (angle_diff > 3.14159f) angle_diff -= 6.28318f;
                while (angle_diff < -3.14159f) angle_diff += 6.28318f;
                m_fly.heading += angle_diff * std::min(1.0f, dt_sec * 12.0f);

                // Move toward target at ~260 px/sec
                float move_step = std::min(dist, 260.0f * dt_sec);
                m_fly.x += std::cos(m_fly.heading) * move_step;
                m_fly.y += std::sin(m_fly.heading) * move_step;
            } else {
                // Arrived at destination tile
                m_fly.x = tx;
                m_fly.y = ty;
                m_fly.current_r = m_fly.target_r;
                m_fly.current_c = m_fly.target_c;

                if (m_fly.pending_action == CellState::Filled) {
                    m_fly.state = FlyActionState::Inking;
                    m_fly.state_timer = 0.30f;
                } else if (m_fly.pending_action == CellState::Crossed) {
                    m_fly.state = FlyActionState::Scratching;
                    m_fly.state_timer = 0.30f;
                } else {
                    m_fly.state = FlyActionState::Idle;
                }
            }
            break;
        }

        case FlyActionState::Inking:
        case FlyActionState::Scratching: {
            m_fly.state_timer -= dt_sec;
            if (m_fly.state_timer <= 0.0f) {
                m_fly.state = FlyActionState::Idle;
                m_fly.pending_action = CellState::Unknown;
            }
            break;
        }

        case FlyActionState::Victory: {
            // High-speed celebratory spin and wing buzz
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

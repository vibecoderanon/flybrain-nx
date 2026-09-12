#include "embodied_fly.hpp"
#include <cmath>
#include <algorithm>

namespace flybrain {

EmbodiedFlyArena::EmbodiedFlyArena() = default;
EmbodiedFlyArena::~EmbodiedFlyArena() = default;

void EmbodiedFlyArena::init() {
    m_fly.x = 0.0f;
    m_fly.y = 0.0f;
    m_fly.heading = 0.0f;
    m_fly.speed = 0.0f;
    m_fly.is_flying = false;
    m_fly.is_grooming = false;

    m_food.clear();
    addFood(150.0f, 120.0f);
    addFood(-180.0f, -100.0f);
    addFood(20.0f, -220.0f);
}

void EmbodiedFlyArena::addFood(float x, float y) {
    FoodPellet f{};
    f.x = x;
    f.y = y;
    f.radius = 18.0f;
    f.consumed = false;
    m_food.push_back(f);
}

void EmbodiedFlyArena::triggerPredatorSwatter(float x, float y) {
    float dist = std::hypot(x - m_fly.x, y - m_fly.y);
    if (dist < 200.0f) {
        // Trigger looming stimulus in the connectome
        // The giant fiber system will decode this and activate escape takeoff
    }
}

void EmbodiedFlyArena::update(LIFEngine& engine, float dt_sec) {
    // 1. Read Motor Commands from Connectome Descending Neurons (DNs)
    float motor_thrust = engine.getMotorThrust();
    float motor_yaw = engine.getMotorYaw();
    bool escape = engine.getMotorEscapeTriggered();

    if (escape) {
        // Emergency ballistic jump
        m_fly.speed = 280.0f; // Rapid escape burst
        m_fly.is_flying = true;
    } else {
        // Normal walking / steering dynamics
        m_fly.speed = std::clamp(motor_thrust * 60.0f, 0.0f, 80.0f);
        m_fly.heading += motor_yaw * 3.5f * dt_sec;
    }

    // Wrap heading to [-pi, pi]
    while (m_fly.heading > 3.14159f) m_fly.heading -= 6.28318f;
    while (m_fly.heading < -3.14159f) m_fly.heading += 6.28318f;

    // Advance physical position
    m_fly.x += std::cos(m_fly.heading) * m_fly.speed * dt_sec;
    m_fly.y += std::sin(m_fly.heading) * m_fly.speed * dt_sec;

    // Arena boundary collision reflection
    float r = std::hypot(m_fly.x, m_fly.y);
    if (r > m_arenaRadius) {
        float normal_angle = std::atan2(m_fly.y, m_fly.x);
        m_fly.x = std::cos(normal_angle) * m_arenaRadius;
        m_fly.y = std::sin(normal_angle) * m_arenaRadius;
        m_fly.heading = normal_angle + 3.14159f + (m_fly.heading - normal_angle) * -0.5f;
    }

    // 2. Feed Sensory Inputs into the Connectome
    // A. Olfactory Gradient: calculate total food odor at antenna position
    float total_odor = 0.0f;
    for (auto& fp : m_food) {
        if (fp.consumed) continue;
        float d = std::hypot(fp.x - m_fly.x, fp.y - m_fly.y);
        if (d < fp.radius) {
            // Taste receptor contact with sugar pellet
            engine.injectTaste(true, 1.0f);
        } else if (d < 300.0f) {
            total_odor += (1.0f - (d / 300.0f));
        }
    }
    if (total_odor > 0.0f) {
        engine.injectOdorPuff(std::min(1.0f, total_odor));
    }

    // B. Visual Optic Flow feedback based on forward speed and angular turn
    float forward_flow = m_fly.speed * 0.01f;
    float yaw_flow = motor_yaw;
    engine.injectOpticFlow(yaw_flow, forward_flow);
}

} // namespace flybrain

#include "sensory_probes.hpp"
#include <cmath>

#ifdef __SWITCH__
#include <switch.h>
static PadState s_pad;
static bool s_padInitialized = false;
#endif

namespace flybrain {

SensoryProbes::SensoryProbes() = default;
SensoryProbes::~SensoryProbes() = default;

void SensoryProbes::init() {
#ifdef __SWITCH__
    if (!s_padInitialized) {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&s_pad);
        s_padInitialized = true;
    }
#endif
}

bool SensoryProbes::pollAndProcess(LIFEngine& engine, BrainRenderer& renderer) {
#ifdef __SWITCH__
    padUpdate(&s_pad);
    u64 kDown = padGetButtonsDown(&s_pad);
    u64 kHeld = padGetButtons(&s_pad);

    if (kDown & HidNpadButton_Plus) {
        return false; // Exit requested
    }

    HidAnalogStickState l_stick = padGetStickPos(&s_pad, 0);
    HidAnalogStickState r_stick = padGetStickPos(&s_pad, 1);

    m_input.left_stick_x = static_cast<float>(l_stick.x) / 32767.0f;
    m_input.left_stick_y = static_cast<float>(l_stick.y) / 32767.0f;
    m_input.right_stick_x = static_cast<float>(r_stick.x) / 32767.0f;
    m_input.right_stick_y = static_cast<float>(r_stick.y) / 32767.0f;

    m_input.trigger_zl = (kHeld & HidNpadButton_ZL) != 0;
    m_input.trigger_zr = (kHeld & HidNpadButton_ZR) != 0;
    m_input.btn_a = (kHeld & HidNpadButton_A) != 0;
    m_input.btn_b = (kHeld & HidNpadButton_B) != 0;
    m_input.btn_x = (kDown & HidNpadButton_X) != 0;
    m_input.btn_y = (kDown & HidNpadButton_Y) != 0;
    m_input.btn_r3 = (kDown & HidNpadButton_StickR) != 0;
    m_input.dpad_up = (kDown & HidNpadButton_Up) != 0;
    m_input.dpad_down = (kDown & HidNpadButton_Down) != 0;
#endif

    // 1. Cycle Speed Selector with X button
    if (m_input.btn_x && !m_prevBtnX) {
        SpeedMode current = engine.getSpeedMode();
        if (current == SpeedMode::RealTime1kHz) {
            engine.setSpeedMode(SpeedMode::RawFidelity);
        } else if (current == SpeedMode::RawFidelity) {
            engine.setSpeedMode(SpeedMode::HighSpeed);
        } else {
            engine.setSpeedMode(SpeedMode::RealTime1kHz);
        }
    }
    m_prevBtnX = m_input.btn_x;

    // 2. Toggle Synaptic Axon Lines with Y button
    if (m_input.btn_y && !m_prevBtnY) {
        renderer.toggleAxonLines();
    }
    m_prevBtnY = m_input.btn_y;

    // 2. Drive 3D Camera with Right Stick
    const float deadzone = 0.15f;
    float rx = (std::abs(m_input.right_stick_x) > deadzone) ? m_input.right_stick_x : 0.0f;
    float ry = (std::abs(m_input.right_stick_y) > deadzone) ? m_input.right_stick_y : 0.0f;

    // Right stick X orbits yaw, Right stick Y orbits pitch
    renderer.updateCamera(rx * 0.04f, ry * 0.04f, 0.0f);

    // D-Pad Up/Down zooms in/out
    if (m_input.dpad_up) renderer.updateCamera(0.0f, 0.0f, -40.0f);
    if (m_input.dpad_down) renderer.updateCamera(0.0f, 0.0f, 40.0f);

    // 3. Optogenetic Sensory Stimulation
    // Left Stick -> Compound eye optic flow motion (yaw drift & pitch)
    float lx = (std::abs(m_input.left_stick_x) > deadzone) ? m_input.left_stick_x : 0.0f;
    float ly = (std::abs(m_input.left_stick_y) > deadzone) ? m_input.left_stick_y : 0.0f;
    if (lx != 0.0f || ly != 0.0f) {
        engine.injectOpticFlow(lx, ly);
    }

    // ZL -> Sweet taste (proboscis extension / forward approach)
    if (m_input.trigger_zl) {
        engine.injectTaste(true, 1.0f);
    }

    // ZR -> Bitter taste (aversive retreat / stop)
    if (m_input.trigger_zr) {
        engine.injectTaste(false, 1.0f);
    }

    // A Button -> Antennal odor puff (olfactory receptor stimulation)
    if (m_input.btn_a) {
        engine.injectOdorPuff(1.0f);
    }

    // R3 (Click Right Stick) -> Giant Fiber looming predator escape reflex
    if (m_input.btn_r3) {
        engine.injectPredatorLoom();
    }

    return true; // Keep running
}

} // namespace flybrain

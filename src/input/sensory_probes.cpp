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

PicrossInputActions SensoryProbes::poll(BrainRenderer& renderer) {
    PicrossInputActions actions{};

#ifdef __SWITCH__
    padUpdate(&s_pad);
    u64 kDown = padGetButtonsDown(&s_pad);
    u64 kHeld = padGetButtons(&s_pad);

    if (kDown & HidNpadButton_Plus) {
        actions.exit_requested = true;
    }

    HidAnalogStickState l_stick = padGetStickPos(&s_pad, 0);
    HidAnalogStickState r_stick = padGetStickPos(&s_pad, 1);

    float lx = static_cast<float>(l_stick.x) / 32767.0f;
    float ly = static_cast<float>(l_stick.y) / 32767.0f;
    float rx = static_cast<float>(r_stick.x) / 32767.0f;
    float ry = static_cast<float>(r_stick.y) / 32767.0f;

    // D-Pad and Left Stick navigation
    const float deadzone = 0.45f;
    if ((kDown & HidNpadButton_Up) || (ly > deadzone && m_stickRepeatTimerY <= 0.0f)) {
        actions.move_up = true;
        m_stickRepeatTimerY = 0.20f;
    } else if ((kDown & HidNpadButton_Down) || (ly < -deadzone && m_stickRepeatTimerY <= 0.0f)) {
        actions.move_down = true;
        m_stickRepeatTimerY = 0.20f;
    }

    if ((kDown & HidNpadButton_Left) || (lx < -deadzone && m_stickRepeatTimerX <= 0.0f)) {
        actions.move_left = true;
        m_stickRepeatTimerX = 0.20f;
    } else if ((kDown & HidNpadButton_Right) || (lx > deadzone && m_stickRepeatTimerX <= 0.0f)) {
        actions.move_right = true;
        m_stickRepeatTimerX = 0.20f;
    }

    if (std::abs(lx) <= deadzone) m_stickRepeatTimerX = 0.0f;
    else if (m_stickRepeatTimerX > 0.0f) m_stickRepeatTimerX -= 0.016f;

    if (std::abs(ly) <= deadzone) m_stickRepeatTimerY = 0.0f;
    else if (m_stickRepeatTimerY > 0.0f) m_stickRepeatTimerY -= 0.016f;

    // Action buttons
    actions.action_fill = (kDown & HidNpadButton_A) != 0;
    actions.action_cross = (kDown & HidNpadButton_B) != 0;
    actions.toggle_autopilot = (kDown & HidNpadButton_X) != 0;
    actions.toggle_axons = (kDown & HidNpadButton_Y) != 0;
    actions.prev_puzzle = (kDown & (HidNpadButton_L | HidNpadButton_ZL)) != 0;
    actions.next_puzzle = (kDown & (HidNpadButton_R | HidNpadButton_ZR)) != 0;

    // Orbit 3D Brain Camera with Right Stick
    const float cam_deadzone = 0.15f;
    float crx = (std::abs(rx) > cam_deadzone) ? rx : 0.0f;
    float cry = (std::abs(ry) > cam_deadzone) ? ry : 0.0f;
    if (crx != 0.0f || cry != 0.0f) {
        renderer.updateCamera(crx * 0.04f, cry * 0.04f, 0.0f);
    }
#endif

    if (actions.toggle_axons) {
        renderer.toggleAxonLines();
    }

    return actions;
}

} // namespace flybrain

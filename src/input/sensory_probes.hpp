#pragma once

#include "../simulation/lif_engine.hpp"
#include "../graphics/brain_renderer.hpp"
#include <cstdint>

namespace flybrain {

struct InputState {
    float left_stick_x = 0.0f;
    float left_stick_y = 0.0f;
    float right_stick_x = 0.0f;
    float right_stick_y = 0.0f;
    bool trigger_zl = false;
    bool trigger_zr = false;
    bool btn_a = false;
    bool btn_b = false;
    bool btn_x = false;
    bool btn_y = false;
    bool btn_plus = false;
    bool btn_r3 = false;
    bool dpad_up = false;
    bool dpad_down = false;
};

class SensoryProbes {
public:
    SensoryProbes();
    ~SensoryProbes();

    void init();

    /**
     * @brief Polls inputs (from Switch libnx or mock state) and drives simulation & camera
     */
    bool pollAndProcess(LIFEngine& engine, BrainRenderer& renderer);

    const InputState& getCurrentInput() const { return m_input; }

private:
    InputState m_input{};
    bool m_prevBtnX = false;
};

} // namespace flybrain

#pragma once

#include "../simulation/lif_engine.hpp"
#include "../graphics/brain_renderer.hpp"
#include <cstdint>

namespace flybrain {

struct PicrossInputActions {
    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    bool action_fill = false;
    bool action_cross = false;
    bool toggle_autopilot = false;
    bool toggle_axons = false;
    bool prev_puzzle = false;
    bool next_puzzle = false;
    bool exit_requested = false;
};

class SensoryProbes {
public:
    SensoryProbes();
    ~SensoryProbes();

    void init();

    /**
     * @brief Polls inputs (Joy-Con or mock) and returns high-level Picross game actions
     */
    PicrossInputActions poll(BrainRenderer& renderer);

private:
    float m_stickRepeatTimerX = 0.0f;
    float m_stickRepeatTimerY = 0.0f;
    bool m_prevBtnA = false;
    bool m_prevBtnB = false;
    bool m_prevBtnX = false;
    bool m_prevBtnY = false;
    bool m_prevBtnL = false;
    bool m_prevBtnR = false;
};

} // namespace flybrain

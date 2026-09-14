#pragma once

#include "../simulation/lif_engine.hpp"
#include "../picross/picross_board.hpp"
#include <cmath>

namespace flybrain {

enum class FlyActionState : uint8_t {
    Idle = 0,
    Walking = 1,
    Inking = 2,     // Extending proboscis to fill tile
    Scratching = 3, // Scratching front legs to mark 'X'
    Victory = 4     // Puzzle solved celebration spin
};

struct FlyPicrossAvatar {
    float x = 0.0f;       // Screen space pixel X
    float y = 0.0f;       // Screen space pixel Y
    float heading = 0.0f; // Radians
    int current_r = 0;
    int current_c = 0;
    int target_r = 0;
    int target_c = 0;
    FlyActionState state = FlyActionState::Idle;
    float state_timer = 0.0f;
    float wing_phase = 0.0f;
    CellState pending_action = CellState::Unknown;
};

class EmbodiedFlyArena {
public:
    EmbodiedFlyArena();
    ~EmbodiedFlyArena();

    void init();

    /**
     * @brief Update virtual fly kinematics, grid navigation, and inking animations
     */
    void update(LIFEngine& engine, float dt_sec, float tile_size, float grid_origin_x, float grid_origin_y);

    /**
     * @brief Command the fly to navigate to a specific grid coordinate and execute an action
     */
    void commandMove(int r, int c, CellState action, float tile_size, float grid_origin_x, float grid_origin_y);

    void triggerVictory();
    void reset(float start_x, float start_y);

    const FlyPicrossAvatar& getFly() const { return m_fly; }
    FlyPicrossAvatar& getFly() { return m_fly; }

private:
    FlyPicrossAvatar m_fly{};
};

} // namespace flybrain

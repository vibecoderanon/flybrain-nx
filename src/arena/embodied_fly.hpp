#pragma once

#include "../simulation/lif_engine.hpp"
#include "../picross/picross_board.hpp"
#include <cmath>

namespace flybrain {

enum class FlyActionState : uint8_t {
    Idle = 0,
    ScanningLine = 1,
    Walking = 2,
    Inspecting = 3,
    Actuating = 4,
    Victory = 5
};

struct FlyPicrossAvatar {
    float x = 0.0f;       // Screen space pixel X
    float y = 0.0f;       // Screen space pixel Y
    float heading = 0.0f; // Radians
    int current_r = 0;
    int current_c = 0;
    int target_r = 0;
    int target_c = 0;
    int scan_type = -1;   // -1=none, 0=row, 1=col
    int scan_idx = -1;
    FlyActionState state = FlyActionState::Idle;
    float state_timer = 0.0f;
    float wing_phase = 0.0f;
    float proboscis_ext = 0.0f; // 0.0 to 1.0 for proboscis extension
    CellState pending_action = CellState::Unknown;
    bool contact_triggered = false;
    char thought_text[128] = "System initialized. Drosophila connectome ready.";
};

class EmbodiedFlyArena {
public:
    EmbodiedFlyArena();
    ~EmbodiedFlyArena();

    void init();

    /**
     * @brief Update virtual fly kinematics, grid navigation, and inking animations
     */
    void update(LIFEngine& engine, float dt_sec, float tile_size, float grid_origin_x, float grid_origin_y, float speed_mult = 1.0f);

    /**
     * @brief Start full 4-stage biological deliberation cycle
     */
    void startDeliberation(int r, int c, CellState action, int scan_type, int scan_idx,
                           const PicrossBoard& board,
                           float tile_size, float grid_origin_x, float grid_origin_y,
                           float speed_mult = 1.0f);

    /**
     * @brief Direct move command (for manual Joy-Con control)
     */
    void commandMove(int r, int c, CellState action, float tile_size, float grid_origin_x, float grid_origin_y);

    bool hasContactTriggered() const { return m_fly.contact_triggered; }
    void clearContactTriggered() { m_fly.contact_triggered = false; }

    void triggerVictory();
    void reset(float start_x, float start_y);

    const FlyPicrossAvatar& getFly() const { return m_fly; }
    FlyPicrossAvatar& getFly() { return m_fly; }

private:
    FlyPicrossAvatar m_fly{};
};

} // namespace flybrain

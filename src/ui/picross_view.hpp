#pragma once

#include "../picross/picross_board.hpp"
#include "../arena/embodied_fly.hpp"
#include <cstdint>

namespace flybrain {

class PicrossView {
public:
    PicrossView();
    ~PicrossView();

    void init(int panel_width = 640, int panel_height = 680);

    /**
     * @brief Render the interactive Picross board, clues, cursor, and virtual fly onto the left panel
     */
    void render(uint32_t* fb, int screen_width, int screen_height,
                const PicrossBoard& board,
                const std::string& title,
                const std::string& category,
                int cursor_r, int cursor_c,
                bool is_autopilot,
                const EmbodiedFlyArena& arena,
                float solve_time_sec);

    // Coordinate conversion queries for input and fly kinematics
    float getTileSize(int grid_w, int grid_h) const;
    void getGridOrigin(int grid_w, int grid_h, float& out_ox, float& out_oy) const;

private:
    int m_panelWidth = 640;
    int m_panelHeight = 680;

    void drawCell(uint32_t* fb, int screen_w, int screen_h, int x, int y, int size, CellState state, bool is_cursor);
    void drawFly(uint32_t* fb, int screen_w, int screen_h, const FlyPicrossAvatar& fly);
};

} // namespace flybrain

#include "picross_view.hpp"
#include "draw_utils.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace flybrain {

PicrossView::PicrossView() = default;
PicrossView::~PicrossView() = default;

void PicrossView::init(int panel_width, int panel_height) {
    m_panelWidth = panel_width;
    m_panelHeight = panel_height;
}

float PicrossView::getTileSize(int grid_w, int grid_h) const {
    if (grid_w <= 5 && grid_h <= 5) return 56.0f;
    return 36.0f;
}

void PicrossView::getGridOrigin(int grid_w, int grid_h, float& out_ox, float& out_oy) const {
    float tile_size = getTileSize(grid_w, grid_h);
    float grid_total_w = grid_w * tile_size;
    float grid_total_h = grid_h * tile_size;
    float clue_margin_x = (grid_w <= 5) ? 65.0f : 90.0f;
    float clue_margin_y = (grid_h <= 5) ? 65.0f : 90.0f;

    out_ox = (static_cast<float>(m_panelWidth) - grid_total_w + clue_margin_x) * 0.5f;
    out_oy = (static_cast<float>(m_panelHeight) - grid_total_h + clue_margin_y) * 0.5f - 10.0f;
}

void PicrossView::drawCell(uint32_t* fb, int screen_w, int screen_h, int x, int y, int size, CellState state, bool is_cursor) {
    // 1. Cell background
    uint32_t bg_color = 0xFF1E293B; // Deep slate
    DrawUtils::drawRectFilled(fb, screen_w, screen_h, x + 1, y + 1, size - 2, size - 2, bg_color);

    // 2. Cell content
    if (state == CellState::Filled) {
        // Glossy dark cyan/ink with glowing core
        uint32_t ink_color = 0xFF0F172A;
        uint32_t rim_color = 0xFF38BDF8;
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x + 2, y + 2, size - 4, size - 4, ink_color);
        DrawUtils::drawRectOutline(fb, screen_w, screen_h, x + 3, y + 3, size - 6, size - 6, 2, rim_color);
    } else if (state == CellState::Crossed) {
        // Red / Coral 'X'
        uint32_t x_color = 0xFFEF4444;
        int pad = size / 4;
        DrawUtils::drawLine(fb, screen_w, screen_h, x + pad, y + pad, x + size - pad, y + size - pad, x_color);
        DrawUtils::drawLine(fb, screen_w, screen_h, x + pad + 1, y + pad, x + size - pad + 1, y + size - pad, x_color);
        DrawUtils::drawLine(fb, screen_w, screen_h, x + pad, y + size - pad, x + size - pad, y + pad, x_color);
        DrawUtils::drawLine(fb, screen_w, screen_h, x + pad + 1, y + size - pad, x + size - pad + 1, y + pad, x_color);
    } else {
        // Subtle center dot for unknown empty paper
        int center_x = x + size / 2;
        int center_y = y + size / 2;
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, center_x - 1, center_y - 1, 3, 3, 0xFF334155);
    }

    // 3. Selection Cursor Brackets
    if (is_cursor) {
        uint32_t cur_col = 0xFFF59E0B; // Pulsing Solar Amber
        int bracket_len = size / 4;
        // Top-left
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x, y, bracket_len, 3, cur_col);
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x, y, 3, bracket_len, cur_col);
        // Top-right
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x + size - bracket_len, y, bracket_len, 3, cur_col);
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x + size - 3, y, 3, bracket_len, cur_col);
        // Bottom-left
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x, y + size - 3, bracket_len, 3, cur_col);
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x, y + size - bracket_len, 3, bracket_len, cur_col);
        // Bottom-right
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x + size - bracket_len, y + size - 3, bracket_len, 3, cur_col);
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, x + size - 3, y + size - bracket_len, 3, bracket_len, cur_col);
    }
}

void PicrossView::drawFly(uint32_t* fb, int screen_w, int screen_h, const FlyPicrossAvatar& fly) {
    float fx = fly.x;
    float fy = fly.y;
    float cos_h = std::cos(fly.heading);
    float sin_h = std::sin(fly.heading);

    auto transform = [&](float lx, float ly, int& out_x, int& out_y) {
        out_x = static_cast<int>(fx + (lx * cos_h - ly * sin_h));
        out_y = static_cast<int>(fy + (lx * sin_h + ly * cos_h));
    };

    // 1. Shadow beneath fly
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, static_cast<int>(fx + 3), static_cast<int>(fy + 5), 11, 0x55000000);

    // 2. Translucent Wings (flutters based on wing_phase)
    float wing_offset = std::sin(fly.wing_phase) * 6.0f;
    int w1x, w1y, w2x, w2y;
    transform(-6.0f, -12.0f - wing_offset, w1x, w1y);
    transform(-6.0f,  12.0f + wing_offset, w2x, w2y);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, w1x, w1y, 7, 0x88BAE6FD);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, w2x, w2y, 7, 0x88BAE6FD);

    // 3. Abdomen (striped segments)
    int ax, ay;
    transform(-8.0f, 0.0f, ax, ay);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, ax, ay, 9, 0xFF78350F); // Amber brown
    int ax2, ay2;
    transform(-12.0f, 0.0f, ax2, ay2);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, ax2, ay2, 7, 0xFF451A03); // Dark tail

    // 4. Thorax (chest)
    int tx, ty;
    transform(0.0f, 0.0f, tx, ty);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, tx, ty, 8, 0xFFB45309);

    // 5. Head
    int hx, hy;
    transform(8.0f, 0.0f, hx, hy);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, hx, hy, 6, 0xFF92400E);

    // 6. Compound Eyes (Vivid Ruby Red)
    int e1x, e1y, e2x, e2y;
    transform(9.0f, -4.0f, e1x, e1y);
    transform(9.0f,  4.0f, e2x, e2y);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, e1x, e1y, 3, 0xFFDC2626);
    DrawUtils::drawCircleFilled(fb, screen_w, screen_h, e2x, e2y, 3, 0xFFDC2626);

    // 7. Proboscis (extended when inking)
    if (fly.state == FlyActionState::Inking) {
        int px, py;
        transform(14.0f, 0.0f, px, py);
        DrawUtils::drawCircleFilled(fb, screen_w, screen_h, px, py, 4, 0xFF0284C7); // Ink drop
    }
}

void PicrossView::render(uint32_t* fb, int screen_w, int screen_h,
                         const PicrossBoard& board,
                         const std::string& title,
                         const std::string& category,
                         int cursor_r, int cursor_c,
                         bool is_autopilot,
                         const EmbodiedFlyArena& arena,
                         float solve_time_sec) {
    if (!fb) return;

    // 1. Clear Left Viewport Area (0 to 640, 0 to 680)
    uint32_t panel_bg = 0xFF0F172A; // Obsidian Slate
    DrawUtils::drawRectFilled(fb, screen_w, screen_h, 0, 0, m_panelWidth, m_panelHeight, panel_bg);

    // Vertical dividing line between Picross panel and 3D brain
    DrawUtils::drawLine(fb, screen_w, screen_h, m_panelWidth - 1, 0, m_panelWidth - 1, m_panelHeight, 0xFF334155);

    // 2. Header Bar
    char title_buf[128];
    std::snprintf(title_buf, sizeof(title_buf), "%s [%dx%d]", title.c_str(), board.getWidth(), board.getHeight());
    DrawUtils::drawString(fb, screen_w, screen_h, 24, 18, title_buf, 0xFFF8FAFC, 2);

    char tag_buf[64];
    std::snprintf(tag_buf, sizeof(tag_buf), "CATEGORY: %s", category.c_str());
    DrawUtils::drawString(fb, screen_w, screen_h, 24, 44, tag_buf, 0xFF94A3B8, 1);

    if (is_autopilot) {
        DrawUtils::drawString(fb, screen_w, screen_h, 300, 44, "[AUTOPILOT: FLY SOLVING]", 0xFF38BDF8, 1);
    } else {
        DrawUtils::drawString(fb, screen_w, screen_h, 300, 44, "[MANUAL PLAY: JOY-CON]", 0xFF4ADE80, 1);
    }

    // 3. Grid Geometry
    int gw = board.getWidth();
    int gh = board.getHeight();
    float tile_size_f = getTileSize(gw, gh);
    int tile_size = static_cast<int>(tile_size_f);

    float ox_f, oy_f;
    getGridOrigin(gw, gh, ox_f, oy_f);
    int ox = static_cast<int>(ox_f);
    int oy = static_cast<int>(oy_f);

    // 4. Render Top Clues (for each column)
    int clue_font_scale = (gw <= 5) ? 2 : 1;
    int clue_h_step = (gw <= 5) ? 18 : 12;

    for (int c = 0; c < gw; ++c) {
        const auto& clues = board.getColClues(c);
        bool satisfied = board.isColSatisfied(c);
        uint32_t clue_col = satisfied ? 0xFF475569 : 0xFFF8FAFC;

        int num_clues = static_cast<int>(clues.size());
        int cx = ox + c * tile_size + (tile_size / 2) - 4 * clue_font_scale;
        int base_y = oy - 6;

        for (int i = 0; i < num_clues; ++i) {
            int clue_val = clues[num_clues - 1 - i];
            int cy = base_y - (i + 1) * clue_h_step;
            char num_str[8];
            std::snprintf(num_str, sizeof(num_str), "%d", clue_val);
            DrawUtils::drawString(fb, screen_w, screen_h, cx, cy, num_str, clue_col, clue_font_scale);

            if (satisfied) {
                // Strikethrough line
                DrawUtils::drawLine(fb, screen_w, screen_h, cx - 2, cy + 4 * clue_font_scale, cx + 10 * clue_font_scale, cy + 4 * clue_font_scale, 0xFF64748B);
            }
        }
    }

    // 5. Render Left Clues (for each row)
    int clue_w_step = (gw <= 5) ? 18 : 12;

    for (int r = 0; r < gh; ++r) {
        const auto& clues = board.getRowClues(r);
        bool satisfied = board.isRowSatisfied(r);
        uint32_t clue_col = satisfied ? 0xFF475569 : 0xFFF8FAFC;

        int num_clues = static_cast<int>(clues.size());
        int ry = oy + r * tile_size + (tile_size / 2) - 4 * clue_font_scale;
        int base_x = ox - 10;

        for (int i = 0; i < num_clues; ++i) {
            int clue_val = clues[num_clues - 1 - i];
            int rx = base_x - (i + 1) * clue_w_step;
            char num_str[8];
            std::snprintf(num_str, sizeof(num_str), "%d", clue_val);
            DrawUtils::drawString(fb, screen_w, screen_h, rx, ry, num_str, clue_col, clue_font_scale);

            if (satisfied) {
                // Strikethrough line
                DrawUtils::drawLine(fb, screen_w, screen_h, rx - 1, ry + 4 * clue_font_scale, rx + 9 * clue_font_scale, ry + 4 * clue_font_scale, 0xFF64748B);
            }
        }
    }

    // 6. Render Grid Cells
    for (int r = 0; r < gh; ++r) {
        for (int c = 0; c < gw; ++c) {
            int cx = ox + c * tile_size;
            int cy = oy + r * tile_size;
            bool is_cur = (r == cursor_r && c == cursor_c && !is_autopilot);
            drawCell(fb, screen_w, screen_h, cx, cy, tile_size, board.getCell(r, c), is_cur);
        }
    }

    // 7. Render Grid Borders and Major 5x5 Grid Dividers
    uint32_t grid_border = 0xFF475569;
    uint32_t major_divider = 0xFF94A3B8;

    int total_w = gw * tile_size;
    int total_h = gh * tile_size;
    DrawUtils::drawRectOutline(fb, screen_w, screen_h, ox, oy, total_w, total_h, 2, grid_border);

    // Thick dividers every 5 cells
    for (int r = 5; r < gh; r += 5) {
        int div_y = oy + r * tile_size;
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, ox, div_y - 1, total_w, 2, major_divider);
    }
    for (int c = 5; c < gw; c += 5) {
        int div_x = ox + c * tile_size;
        DrawUtils::drawRectFilled(fb, screen_w, screen_h, div_x - 1, oy, 2, total_h, major_divider);
    }

    // 8. Render Embodied Fly Avatar
    drawFly(fb, screen_w, screen_h, arena.getFly());

    // 9. Victory Banner Overlay
    if (board.isSolved()) {
        int banner_w = 440;
        int banner_h = 70;
        int bx = (m_panelWidth - banner_w) / 2;
        int by = oy + total_h / 2 - banner_h / 2;

        DrawUtils::drawRectFilled(fb, screen_w, screen_h, bx, by, banner_w, banner_h, 0xEE0B0F19);
        DrawUtils::drawRectOutline(fb, screen_w, screen_h, bx, by, banner_w, banner_h, 2, 0xFFF59E0B);

        char solve_buf[64];
        if (is_autopilot) {
            std::snprintf(solve_buf, sizeof(solve_buf), "DROSOPHILA CONNECTOME SOLVED! (%.1fs)", solve_time_sec);
        } else {
            std::snprintf(solve_buf, sizeof(solve_buf), "PUZZLE SOLVED! TIME: %.1fs", solve_time_sec);
        }
        DrawUtils::drawString(fb, screen_w, screen_h, bx + 16, by + 16, solve_buf, 0xFFFBBF24, 1);
        DrawUtils::drawString(fb, screen_w, screen_h, bx + 16, by + 40, "Press [L] or [R] for Next Puzzle", 0xFFE2E8F0, 1);
    }
}

} // namespace flybrain

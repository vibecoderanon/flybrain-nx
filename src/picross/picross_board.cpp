#include "picross_board.hpp"
#include <algorithm>
#include <numeric>
#include <functional>

namespace flybrain {

PicrossBoard::PicrossBoard() = default;

PicrossBoard::PicrossBoard(int width, int height,
                           const std::vector<std::vector<int>>& row_clues,
                           const std::vector<std::vector<int>>& col_clues,
                           const std::vector<CellState>& solution) {
    init(width, height, row_clues, col_clues, solution);
}

void PicrossBoard::init(int width, int height,
                        const std::vector<std::vector<int>>& row_clues,
                        const std::vector<std::vector<int>>& col_clues,
                        const std::vector<CellState>& solution) {
    m_width = width;
    m_height = height;
    m_rowClues = row_clues;
    m_colClues = col_clues;
    m_solution = solution;
    m_grid.assign(width * height, CellState::Unknown);
}

void PicrossBoard::reset() {
    std::fill(m_grid.begin(), m_grid.end(), CellState::Unknown);
}

CellState PicrossBoard::getCell(int r, int c) const {
    if (r < 0 || r >= m_height || c < 0 || c >= m_width) return CellState::Unknown;
    return m_grid[r * m_width + c];
}

void PicrossBoard::setCell(int r, int c, CellState state) {
    if (r >= 0 && r < m_height && c >= 0 && c < m_width) {
        m_grid[r * m_width + c] = state;
    }
}

void PicrossBoard::toggleFill(int r, int c) {
    if (r < 0 || r >= m_height || c < 0 || c >= m_width) return;
    int idx = r * m_width + c;
    if (m_grid[idx] == CellState::Filled) {
        m_grid[idx] = CellState::Unknown;
    } else {
        m_grid[idx] = CellState::Filled;
    }
}

void PicrossBoard::toggleCross(int r, int c) {
    if (r < 0 || r >= m_height || c < 0 || c >= m_width) return;
    int idx = r * m_width + c;
    if (m_grid[idx] == CellState::Crossed) {
        m_grid[idx] = CellState::Unknown;
    } else {
        m_grid[idx] = CellState::Crossed;
    }
}

const std::vector<int>& PicrossBoard::getRowClues(int r) const {
    static const std::vector<int> empty;
    if (r < 0 || r >= static_cast<int>(m_rowClues.size())) return empty;
    return m_rowClues[r];
}

const std::vector<int>& PicrossBoard::getColClues(int c) const {
    static const std::vector<int> empty;
    if (c < 0 || c >= static_cast<int>(m_colClues.size())) return empty;
    return m_colClues[c];
}

std::vector<int> PicrossBoard::extractRuns(const std::vector<CellState>& line) {
    std::vector<int> runs;
    int current_len = 0;
    for (CellState s : line) {
        if (s == CellState::Filled) {
            current_len++;
        } else {
            if (current_len > 0) {
                runs.push_back(current_len);
                current_len = 0;
            }
        }
    }
    if (current_len > 0) {
        runs.push_back(current_len);
    }
    return runs;
}

bool PicrossBoard::checkLineSatisfied(const std::vector<CellState>& line, const std::vector<int>& clues) {
    std::vector<int> clean_clues;
    for (int c : clues) {
        if (c > 0) clean_clues.push_back(c);
    }
    std::vector<int> runs = extractRuns(line);
    return runs == clean_clues;
}

bool PicrossBoard::checkLineContradiction(const std::vector<CellState>& line, const std::vector<int>& clues) {
    std::vector<int> clean_clues;
    int target_total = 0;
    int max_clue = 0;
    for (int c : clues) {
        if (c > 0) {
            clean_clues.push_back(c);
            target_total += c;
            max_clue = std::max(max_clue, c);
        }
    }

    int current_filled = 0;
    int current_run = 0;
    int run_count = 0;

    for (CellState s : line) {
        if (s == CellState::Filled) {
            current_filled++;
            current_run++;
            if (current_run > max_clue) return true; // Single run exceeds maximum clue
        } else {
            if (current_run > 0) {
                run_count++;
                current_run = 0;
            }
        }
    }
    if (current_run > 0) run_count++;

    if (current_filled > target_total) return true; // More filled tiles than allowed

    return false;
}

bool PicrossBoard::isRowSatisfied(int r) const {
    if (r < 0 || r >= m_height) return false;
    std::vector<CellState> line(m_width);
    for (int c = 0; c < m_width; ++c) {
        line[c] = m_grid[r * m_width + c];
    }
    return checkLineSatisfied(line, m_rowClues[r]);
}

bool PicrossBoard::isColSatisfied(int c) const {
    if (c < 0 || c >= m_width) return false;
    std::vector<CellState> line(m_height);
    for (int r = 0; r < m_height; ++r) {
        line[r] = m_grid[r * m_width + c];
    }
    return checkLineSatisfied(line, m_colClues[c]);
}

bool PicrossBoard::isRowContradiction(int r) const {
    if (r < 0 || r >= m_height) return false;
    std::vector<CellState> line(m_width);
    for (int c = 0; c < m_width; ++c) {
        line[c] = m_grid[r * m_width + c];
    }
    return checkLineContradiction(line, m_rowClues[r]);
}

bool PicrossBoard::isColContradiction(int c) const {
    if (c < 0 || c >= m_width) return false;
    std::vector<CellState> line(m_height);
    for (int r = 0; r < m_height; ++r) {
        line[r] = m_grid[r * m_width + c];
    }
    return checkLineContradiction(line, m_colClues[c]);
}

bool PicrossBoard::isSolved() const {
    for (int r = 0; r < m_height; ++r) {
        if (!isRowSatisfied(r)) return false;
    }
    for (int c = 0; c < m_width; ++c) {
        if (!isColSatisfied(c)) return false;
    }
    return true;
}

int PicrossBoard::getFilledCount() const {
    int count = 0;
    for (CellState s : m_grid) {
        if (s == CellState::Filled) count++;
    }
    return count;
}

int PicrossBoard::getTargetFilledCount() const {
    int sum = 0;
    for (const auto& row : m_rowClues) {
        for (int c : row) sum += c;
    }
    return sum;
}

float PicrossBoard::getProgress() const {
    int target = getTargetFilledCount();
    if (target == 0) return isSolved() ? 1.0f : 0.0f;
    return std::clamp(static_cast<float>(getFilledCount()) / static_cast<float>(target), 0.0f, 1.0f);
}

CellState PicrossBoard::getSolutionCell(int r, int c) const {
    if (r < 0 || r >= m_height || c < 0 || c >= m_width || m_solution.empty()) return CellState::Unknown;
    return m_solution[r * m_width + c];
}

bool PicrossBoard::deduceLine(const std::vector<CellState>& current_line,
                              const std::vector<int>& clues,
                              std::vector<CellState>& deduced_line) {
    int n = static_cast<int>(current_line.size());
    deduced_line.assign(n, CellState::Unknown);

    std::vector<int> clean_clues;
    for (int c : clues) {
        if (c > 0) clean_clues.push_back(c);
    }

    // If clues is empty, every valid cell must be Crossed
    if (clean_clues.empty()) {
        for (int i = 0; i < n; ++i) {
            if (current_line[i] != CellState::Crossed) {
                deduced_line[i] = CellState::Crossed;
            }
        }
        return true;
    }

    std::vector<std::vector<CellState>> valid_configs;

    // Helper to test if a candidate line is compatible with current_line
    auto is_compatible = [&](const std::vector<CellState>& cand) {
        for (int i = 0; i < n; ++i) {
            if (current_line[i] == CellState::Filled && cand[i] != CellState::Filled) return false;
            if (current_line[i] == CellState::Crossed && cand[i] == CellState::Filled) return false;
        }
        return true;
    };

    // Recursive placement of clues
    std::vector<CellState> candidate(n, CellState::Crossed);
    std::function<void(int, int)> generate = [&](int clue_idx, int start_pos) {
        if (clue_idx >= static_cast<int>(clean_clues.size())) {
            if (is_compatible(candidate)) {
                valid_configs.push_back(candidate);
            }
            return;
        }

        int block_len = clean_clues[clue_idx];
        // Calculate minimum space needed for remaining blocks
        int remaining_space = 0;
        for (int k = clue_idx + 1; k < static_cast<int>(clean_clues.size()); ++k) {
            remaining_space += 1 + clean_clues[k];
        }

        int max_pos = n - remaining_space - block_len;
        for (int p = start_pos; p <= max_pos; ++p) {
            // Check if placing block at p violates existing Crossed markings
            bool can_place = true;
            for (int i = 0; i < block_len; ++i) {
                if (current_line[p + i] == CellState::Crossed) {
                    can_place = false;
                    break;
                }
            }
            // Check delimiter after block
            if (can_place && (p + block_len < n) && current_line[p + block_len] == CellState::Filled) {
                can_place = false;
            }

            if (can_place) {
                // Apply block
                for (int i = 0; i < block_len; ++i) {
                    candidate[p + i] = CellState::Filled;
                }

                generate(clue_idx + 1, p + block_len + 1);

                // Revert block
                for (int i = 0; i < block_len; ++i) {
                    candidate[p + i] = CellState::Crossed;
                }
            }
        }
    };

    generate(0, 0);

    if (valid_configs.empty()) {
        return false; // No valid configuration found (contradiction present)
    }

    // Intersect valid configurations
    for (int i = 0; i < n; ++i) {
        bool always_filled = true;
        bool always_crossed = true;

        for (const auto& cfg : valid_configs) {
            if (cfg[i] != CellState::Filled) always_filled = false;
            if (cfg[i] == CellState::Filled) always_crossed = false;
        }

        if (always_filled) {
            deduced_line[i] = CellState::Filled;
        } else if (always_crossed) {
            deduced_line[i] = CellState::Crossed;
        } else {
            deduced_line[i] = CellState::Unknown;
        }
    }

    return true;
}

bool PicrossBoard::findNextDeduction(int& out_r, int& out_c, CellState& out_state) const {
    // 1. Check rows
    for (int r = 0; r < m_height; ++r) {
        std::vector<CellState> cur_row(m_width);
        for (int c = 0; c < m_width; ++c) cur_row[c] = getCell(r, c);

        std::vector<CellState> deduced;
        if (deduceLine(cur_row, m_rowClues[r], deduced)) {
            for (int c = 0; c < m_width; ++c) {
                if (cur_row[c] == CellState::Unknown && deduced[c] != CellState::Unknown) {
                    out_r = r;
                    out_c = c;
                    out_state = deduced[c];
                    return true;
                }
            }
        }
    }

    // 2. Check columns
    for (int c = 0; c < m_width; ++c) {
        std::vector<CellState> cur_col(m_height);
        for (int r = 0; r < m_height; ++r) cur_col[r] = getCell(r, c);

        std::vector<CellState> deduced;
        if (deduceLine(cur_col, m_colClues[c], deduced)) {
            for (int r = 0; r < m_height; ++r) {
                if (cur_col[r] == CellState::Unknown && deduced[r] != CellState::Unknown) {
                    out_r = r;
                    out_c = c;
                    out_state = deduced[r];
                    return true;
                }
            }
        }
    }

    // 3. Fallback: if ground truth solution exists, find first unrevealed solution cell
    if (!m_solution.empty()) {
        for (int r = 0; r < m_height; ++r) {
            for (int c = 0; c < m_width; ++c) {
                if (getCell(r, c) == CellState::Unknown) {
                    out_r = r;
                    out_c = c;
                    out_state = m_solution[r * m_width + c];
                    return true;
                }
            }
        }
    }

    return false;
}

} // namespace flybrain

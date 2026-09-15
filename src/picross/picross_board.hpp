#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace flybrain {

enum class CellState : uint8_t {
    Unknown = 0, // Blank / unmarked tile
    Filled  = 1, // Inked black tile
    Crossed = 2  // Marked with 'X' (known empty)
};

class PicrossBoard {
public:
    PicrossBoard();
    PicrossBoard(int width, int height,
                 const std::vector<std::vector<int>>& row_clues,
                 const std::vector<std::vector<int>>& col_clues,
                 const std::vector<CellState>& solution = {});

    void init(int width, int height,
              const std::vector<std::vector<int>>& row_clues,
              const std::vector<std::vector<int>>& col_clues,
              const std::vector<CellState>& solution = {});

    void reset();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    CellState getCell(int r, int c) const;
    void setCell(int r, int c, CellState state);
    void toggleFill(int r, int c);
    void toggleCross(int r, int c);

    const std::vector<int>& getRowClues(int r) const;
    const std::vector<int>& getColClues(int c) const;

    const std::vector<std::vector<int>>& getAllRowClues() const { return m_rowClues; }
    const std::vector<std::vector<int>>& getAllColClues() const { return m_colClues; }

    bool isRowSatisfied(int r) const;
    bool isColSatisfied(int c) const;
    bool isRowContradiction(int r) const;
    bool isColContradiction(int c) const;

    bool isSolved() const;
    int getFilledCount() const;
    int getTargetFilledCount() const;
    float getProgress() const;

    // Line logic deduction helpers
    static bool checkLineSatisfied(const std::vector<CellState>& line, const std::vector<int>& clues);
    static bool checkLineContradiction(const std::vector<CellState>& line, const std::vector<int>& clues);
    static std::vector<int> extractRuns(const std::vector<CellState>& line);

    // Compute line-logic overlap deductions for a single line
    static bool deduceLine(const std::vector<CellState>& current_line,
                           const std::vector<int>& clues,
                           std::vector<CellState>& deduced_line);

    // Find next deterministic logical deduction for the entire board
    // Returns true if a deduction was found, and sets out_r, out_c, out_state
    bool findNextDeduction(int& out_r, int& out_c, CellState& out_state) const;
    bool findNextDeductionEx(int& out_r, int& out_c, CellState& out_state, int& out_scan_type, int& out_scan_idx) const;

    void setActiveScan(int scan_type, int scan_idx) { m_activeScanType = scan_type; m_activeScanIdx = scan_idx; }
    int getActiveScanType() const { return m_activeScanType; } // -1=none, 0=row, 1=col
    int getActiveScanIdx() const { return m_activeScanIdx; }

    // Ground truth solution (if available)
    bool hasSolution() const { return !m_solution.empty(); }
    CellState getSolutionCell(int r, int c) const;

private:
    int m_width = 0;
    int m_height = 0;
    std::vector<CellState> m_grid;
    std::vector<std::vector<int>> m_rowClues;
    std::vector<std::vector<int>> m_colClues;
    std::vector<CellState> m_solution;
    int m_activeScanType = -1; // -1 = none, 0 = row, 1 = col
    int m_activeScanIdx = -1;
};

} // namespace flybrain

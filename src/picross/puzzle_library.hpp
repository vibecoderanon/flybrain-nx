#pragma once

#include "picross_board.hpp"
#include <vector>
#include <string>

namespace flybrain {

struct PuzzleDef {
    std::string id;
    std::string title;
    std::string category;
    int width;
    int height;
    std::vector<std::vector<int>> row_clues;
    std::vector<std::vector<int>> col_clues;
    std::vector<CellState> solution;

    PicrossBoard createBoard() const {
        return PicrossBoard(width, height, row_clues, col_clues, solution);
    }
};

class PuzzleLibrary {
public:
    static const std::vector<PuzzleDef>& getAllPuzzles();
    static const PuzzleDef& getPuzzle(size_t index);
    static size_t getPuzzleCount();

    // Helper to generate clues from an ASCII artwork map
    static PuzzleDef createFromAscii(const std::string& id,
                                     const std::string& title,
                                     const std::string& category,
                                     int width, int height,
                                     const std::vector<std::string>& rows);
};

} // namespace flybrain

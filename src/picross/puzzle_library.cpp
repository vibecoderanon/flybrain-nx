#include "puzzle_library.hpp"

namespace flybrain {

PuzzleDef PuzzleLibrary::createFromAscii(const std::string& id,
                                         const std::string& title,
                                         const std::string& category,
                                         int width, int height,
                                         const std::vector<std::string>& rows) {
    PuzzleDef def;
    def.id = id;
    def.title = title;
    def.category = category;
    def.width = width;
    def.height = height;
    def.solution.resize(width * height, CellState::Crossed);

    // 1. Parse solution grid
    for (int r = 0; r < height && r < static_cast<int>(rows.size()); ++r) {
        const std::string& row_str = rows[r];
        for (int c = 0; c < width && c < static_cast<int>(row_str.size()); ++c) {
            char ch = row_str[c];
            if (ch == '#' || ch == 'O' || ch == '1') {
                def.solution[r * width + c] = CellState::Filled;
            } else {
                def.solution[r * width + c] = CellState::Crossed;
            }
        }
    }

    // 2. Generate Row Clues
    def.row_clues.resize(height);
    for (int r = 0; r < height; ++r) {
        std::vector<CellState> row_vec(width);
        for (int c = 0; c < width; ++c) {
            row_vec[c] = def.solution[r * width + c];
        }
        std::vector<int> runs = PicrossBoard::extractRuns(row_vec);
        if (runs.empty()) runs.push_back(0);
        def.row_clues[r] = runs;
    }

    // 3. Generate Col Clues
    def.col_clues.resize(width);
    for (int c = 0; c < width; ++c) {
        std::vector<CellState> col_vec(height);
        for (int r = 0; r < height; ++r) {
            col_vec[r] = def.solution[r * width + c];
        }
        std::vector<int> runs = PicrossBoard::extractRuns(col_vec);
        if (runs.empty()) runs.push_back(0);
        def.col_clues[c] = runs;
    }

    return def;
}

const std::vector<PuzzleDef>& PuzzleLibrary::getAllPuzzles() {
    static std::vector<PuzzleDef> puzzles;
    if (puzzles.empty()) {
        // Puzzle 1: Fruit Fly (5x5)
        puzzles.push_back(createFromAscii(
            "fly_5x5", "Fruit Fly", "Biology", 5, 5,
            {
                ".#.#.",
                "#####",
                "..#..",
                ".###.",
                "..#.."
            }
        ));

        // Puzzle 2: Heart SOUL (5x5)
        puzzles.push_back(createFromAscii(
            "heart_5x5", "Heart SOUL", "Symbol", 5, 5,
            {
                ".#.#.",
                "#####",
                "#####",
                ".###.",
                "..#.."
            }
        ));

        // Puzzle 3: Master Sword (5x5)
        puzzles.push_back(createFromAscii(
            "sword_5x5", "Master Sword", "Gaming", 5, 5,
            {
                "..#..",
                "..#..",
                "#####",
                "..#..",
                ".###."
            }
        ));

        // Puzzle 4: Nintendo Switch (5x5)
        puzzles.push_back(createFromAscii(
            "switch_5x5", "Nintendo Switch", "Console", 5, 5,
            {
                "#####",
                "#...#",
                "#...#",
                "#...#",
                "#####"
            }
        ));

        // Puzzle 5: Super Mushroom (5x5)
        puzzles.push_back(createFromAscii(
            "mushroom_5x5", "Super Mushroom", "Gaming", 5, 5,
            {
                ".###.",
                "#####",
                "#.#.#",
                ".###.",
                ".#.#."
            }
        ));

        // Puzzle 6: Drosophila Larva (10x10)
        puzzles.push_back(createFromAscii(
            "larva_10x10", "Drosophila Larva", "Biology", 10, 10,
            {
                "....##....",
                "...####...",
                "..##..##..",
                ".##.##.##.",
                ".########.",
                ".########.",
                "..######..",
                "...####...",
                "....##....",
                ".....#...."
            }
        ));

        // Puzzle 7: Brain Connectome (10x10)
        puzzles.push_back(createFromAscii(
            "brain_10x10", "Brain Connectome", "Neuroscience", 10, 10,
            {
                "..##..##..",
                ".########.",
                "##.####.##",
                "##########",
                "#.##..##.#",
                "####..####",
                ".########.",
                "..######..",
                "...####...",
                "....##...."
            }
        ));

        // Puzzle 8: Retro Space Invader (10x10)
        puzzles.push_back(createFromAscii(
            "invader_10x10", "Space Invader", "Arcade", 10, 10,
            {
                "..#....#..",
                "...#..#...",
                "..######..",
                ".##.##.##.",
                "##########",
                "#.######.#",
                "#.##..##.#",
                "...####...",
                "..##..##..",
                ".##....##."
            }
        ));
    }
    return puzzles;
}

const PuzzleDef& PuzzleLibrary::getPuzzle(size_t index) {
    const auto& list = getAllPuzzles();
    if (index >= list.size()) {
        return list[0];
    }
    return list[index];
}

size_t PuzzleLibrary::getPuzzleCount() {
    return getAllPuzzles().size();
}

} // namespace flybrain

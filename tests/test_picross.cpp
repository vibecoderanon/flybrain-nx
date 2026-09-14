#include "../src/picross/picross_board.hpp"
#include "../src/picross/puzzle_library.hpp"
#include "../src/connectome/connectome_loader.hpp"
#include "../src/simulation/lif_engine.hpp"

#include <iostream>
#include <cassert>
#include <vector>

using namespace flybrain;

void testLineSatisfiedAndContradiction() {
    std::cout << "[Test 1] Testing line satisfaction and contradiction checking..." << std::endl;

    // Line: [Filled, Filled, Crossed, Filled, Crossed] -> clues: [2, 1]
    std::vector<CellState> line1 = { CellState::Filled, CellState::Filled, CellState::Crossed, CellState::Filled, CellState::Crossed };
    std::vector<int> clues1 = { 2, 1 };
    assert(PicrossBoard::checkLineSatisfied(line1, clues1) == true);
    assert(PicrossBoard::checkLineContradiction(line1, clues1) == false);

    // Contradiction: 3 filled when max clue is 2
    std::vector<CellState> line2 = { CellState::Filled, CellState::Filled, CellState::Filled, CellState::Crossed, CellState::Crossed };
    assert(PicrossBoard::checkLineContradiction(line2, clues1) == true);
    assert(PicrossBoard::checkLineSatisfied(line2, clues1) == false);

    // Contradiction: sum of filled tiles exceeds total clues
    std::vector<CellState> line3 = { CellState::Filled, CellState::Filled, CellState::Crossed, CellState::Filled, CellState::Filled };
    assert(PicrossBoard::checkLineContradiction(line3, clues1) == true);

    std::cout << "  -> Line satisfaction and contradiction checks verified.\n";
}

void testLineDeductionLogic() {
    std::cout << "[Test 2] Testing deterministic line deduction solver..." << std::endl;

    // 5 cells, clue [4]: cells 1, 2, 3 must be filled (overlap)
    std::vector<CellState> line(5, CellState::Unknown);
    std::vector<int> clues = { 4 };
    std::vector<CellState> deduced;

    bool ok = PicrossBoard::deduceLine(line, clues, deduced);
    assert(ok);
    assert(deduced[0] == CellState::Unknown);
    assert(deduced[1] == CellState::Filled);
    assert(deduced[2] == CellState::Filled);
    assert(deduced[3] == CellState::Filled);
    assert(deduced[4] == CellState::Unknown);

    // 5 cells, clue [5]: all 5 must be filled
    clues = { 5 };
    ok = PicrossBoard::deduceLine(line, clues, deduced);
    assert(ok);
    for (int i = 0; i < 5; ++i) {
        assert(deduced[i] == CellState::Filled);
    }

    // 5 cells, clue [0]: all 5 must be crossed
    clues = { 0 };
    ok = PicrossBoard::deduceLine(line, clues, deduced);
    assert(ok);
    for (int i = 0; i < 5; ++i) {
        assert(deduced[i] == CellState::Crossed);
    }

    std::cout << "  -> Line deduction overlaps (4-in-5, 5-in-5, 0-in-5) verified.\n";
}

void testAllLibraryPuzzlesSolvability() {
    std::cout << "[Test 3] Verifying all 8 library puzzles solve to 100% completion..." << std::endl;

    const auto& puzzles = PuzzleLibrary::getAllPuzzles();
    assert(puzzles.size() == 8);

    for (size_t idx = 0; idx < puzzles.size(); ++idx) {
        const auto& p = puzzles[idx];
        PicrossBoard board = p.createBoard();

        int max_steps = p.width * p.height * 2;
        int step = 0;
        int r = 0, c = 0;
        CellState action = CellState::Unknown;

        while (!board.isSolved() && step < max_steps) {
            bool found = board.findNextDeduction(r, c, action);
            if (!found) break;
            board.setCell(r, c, action);
            step++;
        }

        bool solved = board.isSolved();
        std::cout << "  -> [" << (solved ? "PASS" : "FAIL") << "] Puzzle " << (idx + 1)
                  << ": " << p.title << " [" << p.width << "x" << p.height << "] solved in "
                  << step << " steps.\n";
        assert(solved);
    }
}

void testNeuromorphicSolverLoop() {
    std::cout << "[Test 4] Testing LIFEngine neuromorphic Picross solver integration..." << std::endl;

    ConnectomeLoader loader;
    loader.loadSyntheticReference(500, 30);

    LIFEngine engine;
    bool init_ok = engine.init(loader);
    assert(init_ok);

    const auto& puzzle = PuzzleLibrary::getPuzzle(0); // Fruit Fly (5x5)
    PicrossBoard board = puzzle.createBoard();
    engine.bindPicrossBoard(&board);

    int max_steps = 100;
    int step = 0;
    int out_r = 0, out_c = 0;
    CellState out_action = CellState::Unknown;

    while (!board.isSolved() && step < max_steps) {
        bool stepped = engine.stepPicrossSolver(out_r, out_c, out_action);
        if (!stepped) break;

        // Step SNN dynamics
        for (int i = 0; i < 4; ++i) {
            engine.step(1.0f);
        }
        step++;
    }

    assert(board.isSolved());
    assert(engine.getPicrossFocusNeuron() < engine.getNeuronCount());
    std::cout << "  -> Neuromorphic LIF solver solved Fruit Fly (5x5) in " << step << " neural decisions.\n";
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "  flybrain-nx: Picross Verification Suite            \n";
    std::cout << "======================================================\n";

    testLineSatisfiedAndContradiction();
    testLineDeductionLogic();
    testAllLibraryPuzzlesSolvability();
    testNeuromorphicSolverLoop();

    std::cout << "\n[+] ALL PICROSS ENGINE & SNN TESTS PASSED SUCCESSFULLY!\n";
    return 0;
}

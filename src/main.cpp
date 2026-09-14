#include "guard/applet_guard.hpp"
#include "connectome/connectome_loader.hpp"
#include "simulation/lif_engine.hpp"
#include "graphics/brain_renderer.hpp"
#include "input/sensory_probes.hpp"
#include "ui/hud.hpp"
#include "ui/picross_view.hpp"
#include "arena/embodied_fly.hpp"
#include "picross/puzzle_library.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#ifdef __SWITCH__
#include <switch.h>
#endif

int main(int argc, char* argv[]) {
    // 1. Strict Title Override Guard Check
    // If launched via Album (Applet Mode), displays warning and halts safely
    if (!flybrain::AppletGuard::enforceTitleOverride()) {
        return 0;
    }

#ifdef __SWITCH__
    romfsInit();
#endif

    // 2. Load Drosophila Connectome
    flybrain::ConnectomeLoader loader;
    std::string connectome_path = "romfs:/drosophila_full.bin";

    std::cout << "[*] flybrain-nx: Initializing connectome..." << std::endl;
    bool loaded = loader.loadFromFile(connectome_path);

    if (!loaded) {
        // Fallback to high-density reference connectome for standalone/PC testing
        std::cout << "[!] Could not load " << connectome_path << " (" << loader.getLastError() << ")\n";
        std::cout << "[*] Generating authentic Drosophila reference model (10,000 neurons, 400,000 synapses)..." << std::endl;
        loader.loadSyntheticReference(10000, 40);
    }

    std::cout << "[+] Connectome Loaded: " << loader.getNeuronCount() << " neurons, "
              << loader.getSynapseCount() << " synapses." << std::endl;
    std::cout << "[+] Memory Footprint: " << (loader.getMemoryFootprintBytes() / (1024 * 1024)) << " MB" << std::endl;

    // 3. Initialize Engine, Graphics, Sensory Probes, HUD, Picross Engine
    flybrain::LIFEngine engine;
    if (!engine.init(loader)) {
        std::cerr << "[-] Failed to initialize LIF simulation engine!" << std::endl;
#ifdef __SWITCH__
        romfsExit();
#endif
        return 1;
    }

    // Default to Real-Time 1kHz mode
    engine.setSpeedMode(flybrain::SpeedMode::RealTime1kHz);

    flybrain::BrainRenderer renderer;
    renderer.init(loader, 1280, 720);

    flybrain::SensoryProbes probes;
    probes.init();

    flybrain::TelemetryHUD hud;
    flybrain::PicrossView picross_view;
    picross_view.init(640, 680);

    flybrain::EmbodiedFlyArena arena;
    arena.init();

    // 4. Setup Picross Puzzle State
    size_t current_puzzle_idx = 0;
    size_t total_puzzles = flybrain::PuzzleLibrary::getPuzzleCount();
    flybrain::PuzzleDef puzzle = flybrain::PuzzleLibrary::getPuzzle(current_puzzle_idx);
    flybrain::PicrossBoard board = puzzle.createBoard();

    engine.bindPicrossBoard(&board);

    int cursor_r = 0;
    int cursor_c = 0;
    bool is_autopilot = true; // Fly Brain Autopilot active by default!
    float autopilot_timer = 0.0f;
    float solve_timer = 0.0f;

    auto resetPuzzleState = [&](size_t idx) {
        current_puzzle_idx = idx % total_puzzles;
        puzzle = flybrain::PuzzleLibrary::getPuzzle(current_puzzle_idx);
        board = puzzle.createBoard();
        engine.bindPicrossBoard(&board);
        cursor_r = 0;
        cursor_c = 0;
        solve_timer = 0.0f;
        autopilot_timer = 0.0f;

        float ox, oy;
        picross_view.getGridOrigin(board.getWidth(), board.getHeight(), ox, oy);
        arena.reset(ox, oy);
    };

    resetPuzzleState(0);

#ifdef __SWITCH__
    // Setup Switch native double-buffered RGBA framebuffer (1280x720)
    NWindow* win = nwindowGetDefault();
    Framebuffer fb;
    framebufferCreate(&fb, win, 1280, 720, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);
#else
    // PC / Host fallback framebuffer buffer
    std::vector<uint32_t> host_fb(1280 * 720, 0);
#endif

    auto prev_time = std::chrono::steady_clock::now();
    float fps = 60.0f;

    // 5. Main Game Loop
#ifdef __SWITCH__
    while (appletMainLoop()) {
#else
    for (int frame = 0; frame < 300; ++frame) { // PC validation test frames
#endif
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        if (dt > 0.0f && dt < 0.2f) {
            fps = (fps * 0.9f) + ((1.0f / dt) * 0.1f);
        } else {
            dt = 0.01667f; // fallback to 60fps delta
        }

        if (!board.isSolved()) {
            solve_timer += dt;
        }

        // A. Poll Controller & Process Picross Actions
        flybrain::PicrossInputActions actions = probes.poll(renderer);
        if (actions.exit_requested) {
            break; // User pressed (+) to exit
        }

        // Puzzle Switching
        if (actions.prev_puzzle) {
            size_t prev_idx = (current_puzzle_idx > 0) ? current_puzzle_idx - 1 : total_puzzles - 1;
            resetPuzzleState(prev_idx);
        } else if (actions.next_puzzle) {
            resetPuzzleState(current_puzzle_idx + 1);
        }

        // Autopilot Toggle
        if (actions.toggle_autopilot) {
            is_autopilot = !is_autopilot;
        }

        float tile_size = picross_view.getTileSize(board.getWidth(), board.getHeight());
        float ox, oy;
        picross_view.getGridOrigin(board.getWidth(), board.getHeight(), ox, oy);

        // B. Game Input Handling
        if (!is_autopilot) {
            // Manual Player Control
            if (actions.move_up && cursor_r > 0) cursor_r--;
            if (actions.move_down && cursor_r < board.getHeight() - 1) cursor_r++;
            if (actions.move_left && cursor_c > 0) cursor_c--;
            if (actions.move_right && cursor_c < board.getWidth() - 1) cursor_c++;

            if (actions.action_fill) {
                board.toggleFill(cursor_r, cursor_c);
                arena.commandMove(cursor_r, cursor_c, flybrain::CellState::Filled, tile_size, ox, oy);
                engine.updatePicrossSensoryFeedback();
                if (board.isSolved()) arena.triggerVictory();
            } else if (actions.action_cross) {
                board.toggleCross(cursor_r, cursor_c);
                arena.commandMove(cursor_r, cursor_c, flybrain::CellState::Crossed, tile_size, ox, oy);
                engine.updatePicrossSensoryFeedback();
                if (board.isSolved()) arena.triggerVictory();
            }
        } else {
            // Autonomous Fly Solver Mode
            autopilot_timer += dt;
            bool force_step = actions.action_fill; // Press A to immediately step fly solver

            if ((autopilot_timer >= 0.45f || force_step) && !board.isSolved()) {
                int out_r = 0, out_c = 0;
                flybrain::CellState out_action = flybrain::CellState::Unknown;
                if (engine.stepPicrossSolver(out_r, out_c, out_action)) {
                    cursor_r = out_r;
                    cursor_c = out_c;
                    arena.commandMove(out_r, out_c, out_action, tile_size, ox, oy);
                    autopilot_timer = 0.0f;
                    if (board.isSolved()) {
                        arena.triggerVictory();
                    }
                }
            }
        }

        // C. Step Leaky Integrate-and-Fire Simulation
        int steps_per_frame = 4;
        for (int step_idx = 0; step_idx < steps_per_frame; ++step_idx) {
            engine.step(1.0f);
        }

        // D. Update Embodied Fly Kinematics on Grid
        arena.update(engine, dt, tile_size, ox, oy);

        // E. Render Split-Screen Viewports & Bottom HUD
        bool lines_on = renderer.areAxonLinesEnabled();
#ifdef __SWITCH__
        u32 stride = 0;
        uint32_t* fb_ptr = reinterpret_cast<uint32_t*>(framebufferBegin(&fb, &stride));
        if (fb_ptr) {
            // Left Panel (0, 0, 640, 680): Picross Board & Fly Avatar
            picross_view.render(fb_ptr, 1280, 720, board, puzzle.title, puzzle.category,
                                cursor_r, cursor_c, is_autopilot, arena, solve_timer);

            // Right Panel (640, 0, 640, 680): 3D Connectome Visualizer & Axon Lines
            renderer.renderSoftware(fb_ptr, 1280, 720, engine, 640, 0, 640, 680);

            // Bottom Bar (0, 680, 1280, 40): Telemetry & Controller Legend
            hud.render(fb_ptr, 1280, 720, engine, fps, lines_on);

            framebufferEnd(&fb);
        }
#else
        picross_view.render(host_fb.data(), 1280, 720, board, puzzle.title, puzzle.category,
                            cursor_r, cursor_c, is_autopilot, arena, solve_timer);
        renderer.renderSoftware(host_fb.data(), 1280, 720, engine, 640, 0, 640, 680);
        hud.render(host_fb.data(), 1280, 720, engine, fps, lines_on);
#endif
    }

    // 6. Cleanup
#ifdef __SWITCH__
    framebufferClose(&fb);
    romfsExit();
#endif

    std::cout << "[*] flybrain-nx: Shutdown complete." << std::endl;
    return 0;
}

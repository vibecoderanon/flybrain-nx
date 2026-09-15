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
    enum class PacingMode : uint8_t {
        Observational = 0, // Default (~2.2s biological thought cycle)
        Fast          = 1, // Rapid (~0.9s cycle)
        StepByStep    = 2, // Pauses at each step; press [A] to advance
        Manual        = 3  // Direct Joy-Con control
    };
    PacingMode pacing_mode = PacingMode::Observational;

    size_t current_puzzle_idx = 0;
    size_t total_puzzles = flybrain::PuzzleLibrary::getPuzzleCount();
    flybrain::PuzzleDef puzzle = flybrain::PuzzleLibrary::getPuzzle(current_puzzle_idx);
    flybrain::PicrossBoard board = puzzle.createBoard();

    engine.bindPicrossBoard(&board);

    int cursor_r = 0;
    int cursor_c = 0;
    float solve_timer = 0.0f;

    auto resetPuzzleState = [&](size_t idx) {
        current_puzzle_idx = idx % total_puzzles;
        puzzle = flybrain::PuzzleLibrary::getPuzzle(current_puzzle_idx);
        board = puzzle.createBoard();
        board.setActiveScan(-1, -1);
        engine.bindPicrossBoard(&board);
        cursor_r = 0;
        cursor_c = 0;
        solve_timer = 0.0f;

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

        // Pacing & Game Mode Cycling (Observational -> Fast -> StepByStep -> Manual)
        if (actions.toggle_autopilot) {
            pacing_mode = static_cast<PacingMode>((static_cast<uint8_t>(pacing_mode) + 1) % 4);
        }

        float tile_size = picross_view.getTileSize(board.getWidth(), board.getHeight());
        float ox, oy;
        picross_view.getGridOrigin(board.getWidth(), board.getHeight(), ox, oy);

        float speed_mult = 1.0f;
        if (pacing_mode == PacingMode::Fast) {
            speed_mult = 2.4f;
        } else if (pacing_mode == PacingMode::StepByStep) {
            speed_mult = 1.2f;
        }

        // B. Game Input Handling & Thought Cycle Deliberation
        if (pacing_mode == PacingMode::Manual) {
            // Manual Player Control
            if (actions.move_up && cursor_r > 0) cursor_r--;
            if (actions.move_down && cursor_r < board.getHeight() - 1) cursor_r++;
            if (actions.move_left && cursor_c > 0) cursor_c--;
            if (actions.move_right && cursor_c < board.getWidth() - 1) cursor_c++;

            if (actions.action_fill) {
                board.toggleFill(cursor_r, cursor_c);
                arena.commandMove(cursor_r, cursor_c, flybrain::CellState::Filled, tile_size, ox, oy);
                engine.commitDeduction(cursor_r, cursor_c, flybrain::CellState::Filled);
                if (board.isSolved()) arena.triggerVictory();
            } else if (actions.action_cross) {
                board.toggleCross(cursor_r, cursor_c);
                arena.commandMove(cursor_r, cursor_c, flybrain::CellState::Crossed, tile_size, ox, oy);
                engine.commitDeduction(cursor_r, cursor_c, flybrain::CellState::Crossed);
                if (board.isSolved()) arena.triggerVictory();
            }
        } else {
            // Autonomous Fly Deliberation Modes (Observational, Fast, StepByStep)
            if (arena.getFly().state == flybrain::FlyActionState::Idle && !board.isSolved()) {
                bool trigger_deduction = true;
                if (pacing_mode == PacingMode::StepByStep) {
                    // In Step mode, press [A] to trigger next deduction
                    trigger_deduction = actions.action_fill;
                }

                if (trigger_deduction) {
                    int out_r = -1, out_c = -1, scan_type = -1, scan_idx = -1;
                    flybrain::CellState out_action = flybrain::CellState::Unknown;
                    if (engine.prepareNextDeduction(out_r, out_c, out_action, scan_type, scan_idx)) {
                        cursor_r = out_r;
                        cursor_c = out_c;
                        board.setActiveScan(scan_type, scan_idx);
                        arena.startDeliberation(out_r, out_c, out_action, scan_type, scan_idx, board, tile_size, ox, oy, speed_mult);
                    } else {
                        board.setActiveScan(-1, -1);
                    }
                }
            }
        }

        // C. Physical-First Board Mutation (Mutates on exact frame proboscis touches paper)
        if (arena.hasContactTriggered()) {
            arena.clearContactTriggered();
            const auto& fly = arena.getFly();
            if (fly.target_r >= 0 && fly.target_c >= 0 && fly.pending_action != flybrain::CellState::Unknown) {
                engine.commitDeduction(fly.target_r, fly.target_c, fly.pending_action);
                board.setActiveScan(-1, -1);
                engine.setOpticScanActive(false);
                if (board.isSolved()) {
                    arena.triggerVictory();
                }
            }
        }

        // D. Step Leaky Integrate-and-Fire Simulation & Decay Spectator Meters
        int steps_per_frame = 4;
        for (int step_idx = 0; step_idx < steps_per_frame; ++step_idx) {
            engine.step(1.0f);
        }
        engine.updateMeters(dt);

        // E. Update Embodied Fly Kinematics on Grid
        arena.update(engine, dt, tile_size, ox, oy, speed_mult);

        // F. Determine Active Cognitive Stage for 3D Brain Spotlight
        int cognitive_phase = 0;
        auto fly_state = arena.getFly().state;
        if (fly_state == flybrain::FlyActionState::ScanningLine) cognitive_phase = 1;
        else if (fly_state == flybrain::FlyActionState::Walking) cognitive_phase = 2;
        else if (fly_state == flybrain::FlyActionState::Inspecting) cognitive_phase = 3;
        else if (fly_state == flybrain::FlyActionState::Actuating) cognitive_phase = 4;

        // G. Render Split-Screen Viewports, Spectator Gauges, and Bottom HUD
        bool lines_on = renderer.areAxonLinesEnabled();
        bool is_autopilot = (pacing_mode != PacingMode::Manual);
        int pace_int = static_cast<int>(pacing_mode);

#ifdef __SWITCH__
        u32 stride = 0;
        uint32_t* fb_ptr = reinterpret_cast<uint32_t*>(framebufferBegin(&fb, &stride));
        if (fb_ptr) {
            // Left Panel (0, 0, 640, 680): Picross Board & Fly Avatar
            picross_view.render(fb_ptr, 1280, 720, board, puzzle.title, puzzle.category,
                                cursor_r, cursor_c, is_autopilot, arena, solve_timer);

            // Right Panel (640, 0, 640, 680): 3D Connectome Visualizer & Axon Lines
            renderer.renderSoftware(fb_ptr, 1280, 720, engine, 640, 0, 640, 680, cognitive_phase, solve_timer, cursor_r, cursor_c);

            // Top Right Spectator Gauges (648, 8, 620, 58): Stonkfly / Doomfly Inspired
            hud.renderSpectatorGauges(fb_ptr, 1280, 720, engine, pace_int);

            // Circuit Highway Signal Flow (648, 68, 620, 24)
            hud.renderCircuitHighway(fb_ptr, 1280, 720, cognitive_phase, solve_timer);

            // Bottom Bar (0, 680, 1280, 40): Telemetry & Controller Legend
            hud.render(fb_ptr, 1280, 720, engine, fps, lines_on);

            framebufferEnd(&fb);
        }
#else
        picross_view.render(host_fb.data(), 1280, 720, board, puzzle.title, puzzle.category,
                            cursor_r, cursor_c, is_autopilot, arena, solve_timer);
        renderer.renderSoftware(host_fb.data(), 1280, 720, engine, 640, 0, 640, 680, cognitive_phase, solve_timer, cursor_r, cursor_c);
        hud.renderSpectatorGauges(host_fb.data(), 1280, 720, engine, pace_int);
        hud.renderCircuitHighway(host_fb.data(), 1280, 720, cognitive_phase, solve_timer);
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

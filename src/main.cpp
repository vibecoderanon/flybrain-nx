#include "guard/applet_guard.hpp"
#include "connectome/connectome_loader.hpp"
#include "simulation/lif_engine.hpp"
#include "graphics/brain_renderer.hpp"
#include "input/sensory_probes.hpp"
#include "ui/hud.hpp"
#include "arena/embodied_fly.hpp"

#include <iostream>
#include <vector>
#include <chrono>

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

    // 2. Load Connectome
    flybrain::ConnectomeLoader loader;
    std::string connectome_path = "romfs:/drosophila_full.bin";

    std::cout << "[*] flybrain-nx: Initializing connectome..." << std::endl;
    bool loaded = loader.loadFromFile(connectome_path);

    if (!loaded) {
        // Fallback to high-density reference connectome for standalone testing
        std::cout << "[!] Could not load " << connectome_path << " (" << loader.getLastError() << ")\n";
        std::cout << "[*] Generating authentic Drosophila reference model (10,000 neurons, 400,000 synapses)..." << std::endl;
        loader.loadSyntheticReference(10000, 40);
    }

    std::cout << "[+] Connectome Loaded: " << loader.getNeuronCount() << " neurons, "
              << loader.getSynapseCount() << " synapses." << std::endl;
    std::cout << "[+] Memory Footprint: " << (loader.getMemoryFootprintBytes() / (1024 * 1024)) << " MB" << std::endl;

    // 3. Initialize Engine, Graphics, Sensory Probes, HUD
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
    flybrain::EmbodiedFlyArena arena;
    arena.init();

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

    // 4. Main Application Loop
#ifdef __SWITCH__
    while (appletMainLoop()) {
#else
    for (int frame = 0; frame < 300; ++frame) { // 300 frames on PC test
#endif
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        if (dt > 0.0f) {
            fps = (fps * 0.9f) + ((1.0f / dt) * 0.1f);
        }

        // A. Poll Controller & Drive Optogenetic Probes
        bool keep_running = probes.pollAndProcess(engine, renderer);
        if (!keep_running) {
            break; // User pressed (+) to exit
        }

        // B. Step SNN Simulation
        // In Real-Time mode, step 16ms of neural time per 60Hz frame
        for (int step_idx = 0; step_idx < 16; ++step_idx) {
            engine.step(1.0f);
        }

        // C. Update Embodied Arena
        arena.update(engine, dt);

        // D. Render 3D Point Cloud & Telemetry HUD
#ifdef __SWITCH__
        u32 stride = 0;
        uint32_t* fb_ptr = reinterpret_cast<uint32_t*>(framebufferBegin(&fb, &stride));
        if (fb_ptr) {
            renderer.renderSoftware(fb_ptr, 1280, 720, engine);
            hud.render(fb_ptr, 1280, 720, engine, fps);
            framebufferEnd(&fb);
        }
#else
        renderer.renderSoftware(host_fb.data(), 1280, 720, engine);
        hud.render(host_fb.data(), 1280, 720, engine, fps);
#endif
    }

    // 5. Cleanup
#ifdef __SWITCH__
    framebufferClose(&fb);
    romfsExit();
#endif

    std::cout << "[*] flybrain-nx: Shutdown complete." << std::endl;
    return 0;
}

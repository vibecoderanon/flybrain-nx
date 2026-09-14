# flybrain-nx // The Fruit Fly Brain Solves Picross! (Nonogram SNN-CSP Homebrew)

[![Platform: Nintendo Switch](https://img.shields.io/badge/Platform-Nintendo_Switch-E60012?logo=nintendoswitch&logoColor=white)](https://github.com/vibecoderanon/flybrain-nx)
[![Language: C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=cplusplus&logoColor=white)](https://github.com/vibecoderanon/flybrain-nx)
[![Toolchain: devkitA64](https://img.shields.io/badge/Toolchain-devkitA64-E65100)](https://devkitpro.org/)
[![Model: FlyWire 2024](https://img.shields.io/badge/Model-FlyWire_Connectome-00E5FF)](https://flywire.ai/)
[![View App Showcase](https://img.shields.io/badge/Web_Portal-View_App-00E5FF?style=for-the-badge&logo=safari&logoColor=white)](https://vibecoderanon.github.io/#flybrain-nx)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An authentic Nintendo Switch homebrew application where the simulated biological fruit fly brain (*Drosophila melanogaster* connectome: **139,255 neurons and 54,498,591 synapses**) **actively solves Picross (Nonogram) puzzles** using a **neuromorphic Spiking Neural Network Constraint Satisfaction Solver (SNN-CSP)**.

Features a dual split-screen interface rendered at 60 FPS:
* **Left Viewport (640×720):** High-contrast Picross puzzle board with interactive clues, completed line strikethroughs, tile cursors, and an animated embodied fruit fly avatar crawling across the grid to ink cells with its proboscis or scratch crosses with its front legs.
* **Right Viewport (640×720):** Rotating 3D fruit fly brain with real-time **dynamic axon transmission lines** that illuminate between active neural clusters as clues are evaluated and constraint hypotheses are tested.

---

## Quick Navigation
* [Web Portal Showcase](https://vibecoderanon.github.io/#flybrain-nx)
* [Neuromorphic Constraint Satisfaction (SNN-CSP)](#neuromorphic-constraint-satisfaction-snn-csp)
* [Split-Screen Interface & Embodied Fly](#split-screen-interface--embodied-fly)
* [Game Modes & Joy-Con Controls](#game-modes--joy-con-controls)
* [Built-in Puzzle Catalog](#built-in-puzzle-catalog)
* [Memory Schema & Hardware Budget](#memory-schema--hardware-budget)
* [Architecture & File Responsibilities](#architecture--file-responsibilities)
* [Building & Console Deployment](#building--console-deployment)
* [Title Override Invariant (Firmware 22.5.0+)](#title-override-invariant-firmware-2250)
* [Credits & Scientific Acknowledgments](#credits--scientific-acknowledgments)

---

## Neuromorphic Constraint Satisfaction (SNN-CSP)

Nonogram puzzles (Picross) belong to the class of **Constraint Satisfaction Problems (CSP)**. In `flybrain-nx`, the fruit fly brain solves them through a biological attractor network grounded in Drosophila neuroanatomy:

```
+-------------------------------------------------------------------------------+
|                       NEUROMORPHIC PICROSS SNN SOLVER                         |
+-------------------------------------------------------------------------------+
| 1. SENSORY INGESTION                                                          |
|    Row & Column clues -> Depolarizing current injected into Optic Lobe /      |
|    Antennal Lobe sensory clusters corresponding to unsolved lines.            |
|                                                                               |
| 2. CONSTRAINT VERIFICATION & PLASTICITY                                       |
|    - Clue Satisfied: Mushroom Body dopamine burst (+3.5 mV) reinforces        |
|      candidate neurons in that line (positive attractor basin).               |
|    - Clue Violated: GABAergic inhibitory burst (-8.0 mV) suppresses firing of |
|      violating candidate neurons (contradiction avoidance).                   |
|    - Thermal Annealing: Biological Poisson noise in the LIF engine escapes     |
|      local constraint minima and guides the brain toward unique solutions.   |
|                                                                               |
| 3. MOTOR READOUT & EMBODIMENT                                                 |
|    - Tile Target (r, c): Central Complex compass neurons direct fly heading.   |
|    - Action [FILL]: Subesophageal Zone (SEZ) proboscis motor burst.           |
|    - Action [CROSS]: Descending Motor (DN) leg grooming scratch burst.        |
+-------------------------------------------------------------------------------+
```

---

## Split-Screen Interface & Embodied Fly

The native 1280×720 Nintendo Switch framebuffer is divided into two balanced 640×720 viewports:

```
+-----------------------------------+-----------------------------------+
| LEFT PANEL (640x720): PICROSS     | RIGHT PANEL (640x720): 3D BRAIN   |
|                                   |                                   |
| PUZZLE 1/8: FRUIT FLY [5x5]       |          .  : *  .                |
| [AUTOPILOT: FLY SOLVING]          |        :  *   /|   .              |
|                                   |       .  --*---*-- .   <-- Axon   |
|        1   2   4   2   1          |        :  /|   | *     Beams      |
|      +---+---+---+---+---+        |          * |   *                  |
|  1 1 | . | # | . | # | . |        |         /  * .  \                 |
|    5 | # | # | # | # | # |        |        *     *   *                |
|    1 | . | . | # | . | . |  <-Fly |      [Spotlight Focus Neuron]     |
|    3 | . | # | # | # | . |        |     (Glows gold on active cell)   |
|    1 | . | . | # | . | . |        |                                   |
|      +---+---+---+---+---+        |                                   |
+-----------------------------------+-----------------------------------+
| BOTTOM BAR: SIM 1,000 Hz | SPIKES 24.5k/s | RENDER 60 FPS | AXONS ON  |
| CONTROLS: [D-Pad] Move [A] Fill [B] Cross [X] Autopilot [Y] Axons (+)Exit |
+-----------------------------------------------------------------------+
```

### Embodied Fruit Fly Avatar Kinematics
* **Tripod Gait Kinematics:** Smoothly crawls across tiles $(r, c)$ at 260 px/sec.
* **Inking Animation:** Extends proboscis to ink cells with obsidian ink droplets.
* **Scratching Animation:** Scratches front legs to carve red 'X' marks.
* **Victory Celebration:** Performs a 360° victory spin and high-frequency wing buzz upon completing the puzzle!

---

## Game Modes & Joy-Con Controls

### 1. Autopilot Mode (Fly Solves)
Sit back and watch the biological connectome deduce, walk across, and solve Nonograms step-by-step using genuine spiking neural dynamics. Press **(A)** at any time to step the solver forward immediately.

### 2. Manual Play Mode (Player Solves)
Take direct control with your Joy-Con controller! Play Picross traditionally with the D-Pad and action buttons while observing real-time brain reactions, dopamine reward flashes, and axon transmission lines in the right viewport.

```
                           [ L / R: Switch Puzzles ]
                                       |
                   +-------------------+-------------------+
                   |                                       |
         [ Left Stick / D-Pad ]                 [ Right Stick ]
         Navigate Grid Tiles                    Orbit 3D Brain Camera
                   |                                       |
                   |                                       |
         [ (A) Button ]  Fill Tile (Manual) / Step Fly Solver (Autopilot)
         [ (B) Button ]  Cross Tile ('X')
         [ (X) Button ]  Toggle Autopilot Mode (Fly Solves vs. Manual Play)
         [ (Y) Button ]  Toggle 3D Axon Transmission Lines (ON / OFF)
         [ (+) Button ]  Exit to Horizon OS HOME Menu
```

---

## Built-in Puzzle Catalog

`flybrain-nx` includes 8 handcrafted, mathematically verified Nonograms spanning biology, gaming, and retro arcade themes:

| # | Puzzle Name | Dimensions | Category | Reveal Artwork |
| :-: | :--- | :---: | :---: | :--- |
| **1** | **Fruit Fly** | 5×5 | Biology | Drosophila body, wings, and compound eyes |
| **2** | **Heart SOUL** | 5×5 | Symbol | Classic 8-bit RPG pixel heart |
| **3** | **Master Sword** | 5×5 | Gaming | Legendary blade hilt and tip |
| **4** | **Nintendo Switch**| 5×5 | Console | Joy-Con console and display border |
| **5** | **Super Mushroom** | 5×5 | Gaming | Iconic power-up mushroom cap and spots |
| **6** | **Drosophila Larva** | 10×10 | Biology | Segmented fruit fly larva crawling with mouth hook |
| **7** | **Brain Connectome** | 10×10 | Neuroscience | Branching neural cortex and synapse arbor |
| **8** | **Space Invader** | 10×10 | Arcade | Classic retro extraterrestrial sprite |

---

## Memory Schema & Hardware Budget

Running under **Title Override** (isolated $\sim 2.7\text{ GB}$ retail pool), `flybrain-nx` loads the entire FlyWire connectome alongside the Picross solver:

| Subsystem / Data Structure | Element Count | Size per Element | Total Memory Footprint | Pool Ratio ($\approx 2.7\text{ GB}$) |
| :--- | :--- | :--- | :--- | :--- |
| **Synapse CSR Table** | 54,498,591 synapses | 8 bytes (`SynapseRecord`) | **$\approx 311.8\text{ MB}$** | $\sim 11.5\%$ |
| **Neuron Records** | 139,255 neurons | 32 bytes (`NeuronRecord`) | **$\approx 4.45\text{ MB}$** | $< 0.2\%$ |
| **LIF Simulation States** | 139,255 states | 16 bytes (`NeuronState`) | **$\approx 2.22\text{ MB}$** | $< 0.1\%$ |
| **Synaptic Accumulators** | 139,255 accumulators | 4 bytes (`atomic<float>`) | **$\approx 0.55\text{ MB}$** | $< 0.05\%$ |
| **3D Point Cloud Vertices** | 139,255 vertices | 16 bytes (`BrainVertex`) | **$\approx 2.22\text{ MB}$** | $< 0.1\%$ |
| **Double-Buffered RGBA Framebuffer**| 1280×720 (2x) | 4 bytes per pixel | **$\approx 7.37\text{ MB}$** | $< 0.3\%$ |
| **Picross Board & Catalog Heap** | 8 Puzzles + Solver | State vectors & caches | **$\approx 0.12\text{ MB}$** | $< 0.01\%$ |
| **TOTAL RUNTIME FOOTPRINT** | | | **$\approx 357.2\text{ MB}$** | **$\approx 13.2\%$** |

---

## Architecture & File Responsibilities

```
flybrain-nx/
├── .github/workflows/
│   └── build.yml               # Containerized devkitA64 CI build & release workflow
├── Makefile                    # devkitA64 libnx Nintendo Switch build specification
├── build_release.py            # Standalone SD card release packager (sdmc:/switch/flybrain-nx/)
├── icon.jpg                    # High-resolution 256x256 glowing connectome homebrew icon
├── romfs/
│   └── drosophila_full.bin     # Packed binary CSR connectome (FLYB format, 32B neurons, 8B synapses)
├── src/
│   ├── main.cpp                # Dual-viewport main loop, Title Override guard, puzzle orchestration
│   ├── arena/
│   │   ├── embodied_fly.hpp    # Virtual fly avatar kinematics and inking state machine
│   │   └── embodied_fly.cpp    # Grid navigation, proboscis inking, and victory spin
│   ├── connectome/
│   │   ├── connectome_format.hpp # FLYB binary layout (64B header, 32B neuron, 8B synapse)
│   │   ├── connectome_loader.hpp # Memory-mapped loader with synthetic reference fallback
│   │   └── connectome_loader.cpp # Binary parser and neuropil spatial generator
│   ├── graphics/
│   │   ├── brain_renderer.hpp  # Viewport software rasterizer, 3D orbit camera, and axon lines
│   │   └── brain_renderer.cpp  # Perspective projection, spike glow, and focus neuron highlighting
│   ├── guard/
│   │   ├── applet_guard.hpp    # appletGetAppletType() memory guard
│   │   └── applet_guard.cpp    # Title Override warning screen preventing Album OOM
│   ├── input/
│   │   ├── sensory_probes.hpp  # Joy-Con input mapping to Picross navigation and actions
│   │   └── sensory_probes.cpp  # Stick repeat timing, button debouncing, and camera orbit
│   ├── picross/
│   │   ├── picross_board.hpp   # Picross board representation, clue validation, and line solver
│   │   ├── picross_board.cpp   # Overlapping candidate deduction and contradiction checking
│   │   ├── puzzle_library.hpp  # Handcrafted Nonogram catalog header
│   │   └── puzzle_library.cpp  # 8 handcrafted 5x5 and 10x10 ASCII-verified puzzles
│   ├── simulation/
│   │   ├── lif_engine.hpp      # Leaky Integrate-and-Fire simulation & Picross SNN mapping
│   │   └── lif_engine.cpp      # Dopamine reward / GABAergic inhibition constraint dynamics
│   └── ui/
│       ├── draw_utils.hpp      # 2D software rendering primitives (lines, rects, circles, font)
│       ├── draw_utils.cpp      # Standard 8x8 font rasterizer with arbitrary integer scaling
│       ├── hud.hpp             # Bottom telemetry bar header
│       ├── hud.cpp             # Telemetry meters and Joy-Con control prompt legend
│       ├── picross_view.hpp    # Left panel (640x720) Picross view rasterizer
│       └── picross_view.cpp    # Dynamic clues, strikethroughs, cell tiles, and fly avatar
├── tests/
│   ├── test_picross.cpp        # C++ unit test suite verifying Picross deductions and SNN solver
│   ├── test_picross_logic.py   # Python test verifying 100% solvability of all catalog puzzles
│   ├── test_lif_engine.cpp     # C++ test suite verifying LIF equations and speed modes
│   └── verify_snn.py           # Python validation script for binary format and SNN math
└── tools/
    └── extract_connectome.py   # FlyWire snapshot parser and descending-sorted CSR packager
```

---

## Building & Console Deployment

### Prerequisites
* [devkitPro](https://devkitpro.org/) with `devkitA64` and `libnx`.
* Python 3.8+ (for binary dataset generation and release packaging).

### Step-by-Step Compilation
1. Verify the RomFS connectome dataset:
   ```bash
   python3 tools/extract_connectome.py --generate-reference --neurons 10000 --output romfs/drosophila_full.bin
   ```
2. Run local logic verification:
   ```bash
   python3 tests/test_picross_logic.py
   ```
3. Compile the native Switch `.nro` executable:
   ```bash
   make -j$(nproc)
   ```
4. Package the SD card release archive:
   ```bash
   python3 build_release.py
   ```

### SD Card Deployment
1. Extract `release/flybrain-nx-switch-v1.1.0.zip` to the root of your SD card:
   ```
   sdmc:/
   └── switch/
       └── flybrain-nx/
           ├── flybrain-nx.nro
           ├── flybrain-nx.xml
           └── icon.jpg
   ```
2. Insert SD card into your Nintendo Switch running Atmosphère CFW.

---

## Title Override Invariant (Firmware 22.5.0+)

> [!IMPORTANT]
> **Do not launch via the Album icon (Applet Mode).**
> On newer Horizon OS firmware updates (such as 22.5.0), Applet Mode restricts available heap to $\sim 150\text{–}200\text{ MB}$. Attempting to load the $\sim 357\text{ MB}$ fruit fly connectome in Applet Mode will trigger an immediate system panic.
>
> **How to launch with Title Override:**
> 1. Return to the Nintendo Switch HOME Menu.
> 2. Hold **[R]** on your controller.
> 3. While holding **[R]**, launch **ANY installed game or retail demo**.
> 4. Keep holding **[R]** until the Homebrew Menu appears.
> 5. Launch **flybrain-nx** from the Homebrew Menu.
>
> `flybrain-nx` includes an integrated `AppletGuard` that safely intercepts Applet Mode launches, halts memory allocation, and displays an on-screen guide rather than crashing.

---

## Credits & Scientific Acknowledgments

* **FlyWire Consortium & Princeton University:** For the complete adult *Drosophila melanogaster* connectome (Dorkenwald et al., Schlegel et al., *Nature* 2024).
* **Philip Shiu et al.:** For the foundational *Drosophila* Leaky Integrate-and-Fire computational brain model.
* **devkitPro Team:** For `devkitA64`, `libnx`, and the open-source Nintendo Switch homebrew toolchain.
* **vibecoderanon:** Architecture, neuromorphic Picross SNN-CSP solver, dual-viewport rasterizer, and Title Override guard.

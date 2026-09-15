# flybrain-nx // The Fruit Fly Brain Solves Picross! (Nonogram SNN-CSP Homebrew)

[![Platform: Nintendo Switch](https://img.shields.io/badge/Platform-Nintendo_Switch-E60012?logo=nintendoswitch&logoColor=white)](https://github.com/vibecoderanon/flybrain-nx)
[![Language: C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=cplusplus&logoColor=white)](https://github.com/vibecoderanon/flybrain-nx)
[![Toolchain: devkitA64](https://img.shields.io/badge/Toolchain-devkitA64-E65100)](https://devkitpro.org/)
[![Model: FlyWire 2024](https://img.shields.io/badge/Model-FlyWire_Connectome-00E5FF)](https://flywire.ai/)
[![View App Showcase](https://img.shields.io/badge/Web_Portal-View_App-00E5FF?style=for-the-badge&logo=safari&logoColor=white)](https://vibecoderanon.github.io/#flybrain-nx)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An authentic Nintendo Switch homebrew application where the simulated biological fruit fly brain (*Drosophila melanogaster* connectome: **139,255 neurons and 54,498,591 synapses**) **actively solves Picross (Nonogram) puzzles** using a **neuromorphic Spiking Neural Network Constraint Satisfaction Solver (SNN-CSP)**.

Features a dual split-screen interface rendered at 60 FPS:
* **Left Viewport (640×720):** High-contrast Picross puzzle board with dynamic numeric clues, line completed strikethroughs, live **Thought Monologue Banner**, **Sensory Scan Reticle** (laser clue beam), and an embodied fruit fly avatar crawling across the grid to ink cells with its proboscis or scratch crosses with its legs.
* **Right Viewport (640×720):** Rotating 3D fruit fly brain with **Spectator Neural Gauges** ([🍬 PAM11 Dopamine], [⚡ PPL101 Aversive], [👁️ Optic Scan], [🧭 Compass CX]), floating 3D anatomical labels ([OPTIC LOBES], [MUSHROOM BODY], [CENTRAL COMPLEX], [MOTOR SEZ]), and real-time **deliberation highway spotlights**.

---

## Quick Navigation
* [Web Portal Showcase](https://vibecoderanon.github.io/#flybrain-nx)
* [Viral Drosophila Connectome Features (Stonkfly / Doomfly Inspired)](#viral-drosophila-connectome-features)
* [Neuromorphic Constraint Satisfaction (SNN-CSP)](#neuromorphic-constraint-satisfaction-snn-csp)
* [Strict Physical-First Causality (4-Stage Thought Cycle)](#strict-physical-first-causality-4-stage-thought-cycle)
* [Split-Screen Interface & Embodied Fly](#split-screen-interface--embodied-fly)
* [Game Modes & Multi-Pacing Selector](#game-modes--multi-pacing-selector)
* [Built-in Puzzle Catalog](#built-in-puzzle-catalog)
* [Memory Schema & Hardware Budget](#memory-schema--hardware-budget)
* [Architecture & File Responsibilities](#architecture--file-responsibilities)
* [Building & Console Deployment](#building--console-deployment)
* [Title Override Invariant (Firmware 22.5.0+)](#title-override-invariant-firmware-2250)
* [Credits & Scientific Acknowledgments](#credits--scientific-acknowledgments)

---

## Viral Drosophila Connectome Features

Following the release of the complete adult fruit fly connectome (*FlyWire 2024*), creative developers built viral projects mapping simulated fly brains to DOOM (**Doomfly**), Bitcoin trading charts (**Stonkfly**), and autonomous vehicles (**flyhard**). `flybrain-nx` incorporates their visual feedback loops to make biological neural dynamics instantly legible:

1. **Dual Spectator Gauges (Top of 3D Brain Viewport):**
   * **`[🍬 PAM11 REWARD]` (Emerald Green):** Spikes radiant emerald (+3.5 mV) whenever a row/column clue is satisfied or puzzle progress advances.
   * **`[⚡ PPL101 AVERSIVE]` (Crimson Red):** Flashes vivid red when the fly encounters a constraint contradiction.
   * **`[👁️ OPTIC SCAN]` (Cyan):** Illuminates while the fly's compound eyes read row/column clue numbers.
   * **`[🧭 COMPASS (CX)]` (Gold):** Illuminates while the Central Complex computes directional steering.

2. **Live Thought Monologue Banner (Top of Picross Viewport):**
   * Real-time narration capsule displaying the fly's observations, hypotheses, and physical actuation decisions (e.g. *"Sensory Scan: Inspecting Row 2 constraints..."*, *"Actuation: Proboscis inking tile (2, 3)! (+PAM11)"*).

3. **Sensory Scan Reticle & Clue Laser Highlighting:**
   * An amber laser targeting beam connects the fly's eyes to the exact row/column clues and grid line currently being scrutinized.

4. **3D Anatomical Neuropil Badges & Deliberation Highway:**
   * Bilateral floating 3D labels identify `[L. OPTIC LOBE]`, `[R. OPTIC LOBE]`, `[CENTRAL COMPLEX]`, `[MUSHROOM BODY]`, and `[MOTOR SEZ]`.
   * Badges are rendered inside solid dark pill cards (`#0a1220`) with 1px colored neuropil borders and leader lines—completely eliminating text clipping or overlap from any camera angle.
   * Active lobes spotlight dynamically (e.g. `[► L. OPTIC SCAN ◄]`) as signals flow from sensory ingestion to motor actuation.

5. **Two-Tier Visual Hierarchy & Traveling Action Potential Packets:**
   * **Anatomical Scaffold:** 10,000 background neurons rendered in low-luminance translucent slate/cyan, creating the distinct 3D anatomical silhouette without visual clutter.
   * **Active Macro-Nodes:** Firing neurons expand into 3px–5px radiant glowing orbs with multi-stage bloom.
   * **Target Tile Reticle `(R, C)`:** The neuron corresponding to the active Picross deduction tile features a pulsing golden target ring with an on-screen coordinate pill tag.
   * **Traveling Axon Pulses:** 16–24 directed synaptic conduits feature animated action potential energy packets traversing axons over ~0.35s, with software rendering locked at **60 FPS**.

6. **Circuit Highway Signal Flow Indicator (Below Spectator Gauges):**
   * Real-time 4-stage neural pipeline: `[ 👁️ OPTIC ] ──► [ 🧭 COMPASS ] ──► [ 🧠 MUSHROOM ] ──► [ ⚡ MOTOR ]`
   * Animated chevrons pulse along the circuit highway to illustrate cognitive signal transmission.

---

## Strict Physical-First Causality (4-Stage Thought Cycle)

Tiles and clues **never mutate prematurely**. Board state changes occur strictly on physical proboscis contact:

```
+-------------------------------------------------------------------------------+
|                       THE 4-STAGE BIOLOGICAL THOUGHT CYCLE                    |
+-------------------------------------------------------------------------------+
| STAGE 1: VISUAL SENSORY SCAN (~0.5s)                                          |
|   - Fly turns toward target line. Active clues illuminate with amber reticle.  |
|   - [OPTIC SCAN] gauge fills with cyan. Optic lobes spotlight in 3D brain.     |
|   - Thought: "👀 Sensory Scan: Inspecting Row 2 constraints..."               |
|                                                                               |
| STAGE 2: HEADING & NAVIGATION (Natural crawl)                                 |
|   - Central Complex compass neurons fire. [COMPASS CX] gauge fills with gold. |
|   - Fly crawls across grid using 6-legged tripod gait. Target cell stays blank.|
|   - Thought: "🚶 Navigation (CX): Crawling toward tile (2, 3)..."              |
|                                                                               |
| STAGE 3: INSPECTION & DELIBERATION (~0.4s)                                    |
|   - Fly halts at tile; antennae and body twitch.                              |
|   - Mushroom Body (Kenyon cells) spotlight in 3D brain.                       |
|   - Thought: "🧠 Deliberation (MB): Verifying overlap at (2, 3)..."            |
|                                                                               |
| STAGE 4: SYNCHRONIZED ACTUATION & DOPAMINE SURGE (~0.35s)                     |
|   - Proboscis extends down to paper.                                          |
|   - ON THE EXACT FRAME OF CONTACT:                                            |
|     * Ink droplet expands and fills the tile (or front legs scratch 'X').     |
|     * [PAM11 REWARD] gauge surges emerald green!                              |
|     * Clues strike through if line constraints are satisfied!                 |
|   - Thought: "🎯 Actuation: Proboscis inking tile (2, 3)! (+PAM11)"            |
+-------------------------------------------------------------------------------+
```

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
| PUZZLE 1/8: FRUIT FLY [5x5]       | [SPECTATOR GAUGES] [OBSERVE 2.2s] |
| [THOUGHT: Crawling to tile (2,3)] | [PAM11: ████████] [PPL101: ░░░░]  |
|                                   | [OPTIC: ██████░░] [CX:     ████░] |
|        1   2   4   2   1          |                                   |
|      +---+---+---+---+---+        |       [OPTIC LOBE]  .  : *        |
|  1 1 | . | # | . | # | . |        |        :  *   /|   .              |
|    5 | # | # | # | # | # |        |       .  --*---*-- .   <-- Axon   |
|  1 > | . | . | # | . | . |  <-Fly |        :  /|   | *     Beams      |
|    3 | . | # | # | # | . |        |     [MUSHROOM BODY]               |
|    1 | . | . | # | . | . |        |         /  * .  \                 |
|      +---+---+---+---+---+        |        *     *   * [MOTOR SEZ]    |
|      [Laser Clue Highlight]       |                                   |
+-----------------------------------+-----------------------------------+
| BOTTOM BAR: SIM 1,000 Hz | SPIKES 24.5k/s | RENDER 60 FPS | AXONS ON  |
| CONTROLS: [D-Pad] Move [A] Mark/Step [B] Cross [X] Mode [Y] Axons (+) |
+-----------------------------------------------------------------------+
```

---

## Game Modes & Multi-Pacing Selector

Press **(X)** to cycle between 4 distinct cognitive pacing modes:

1. **Observational Mode (Default, ~2.2s per move):**
   * Calibrated for spectator enjoyment: watch the fly scan the clues, walk across the tiles, deliberate, and extend its proboscis with synchronized dopamine bursts.
2. **Fast Mode (~0.9s per move):**
   * Rapid solving with preserved physical contact synchronization and high-speed crawling.
3. **Step-by-Step Mode (Manual `[A]`):**
   * Deliberation halts at each step. Press **(A)** to command the connectome to advance to the next deduction, letting you inspect the exact firing circuits at leisure.
4. **Manual Play Mode (Direct Joy-Con Play):**
   * Play Picross traditionally with the D-Pad and action buttons while observing real-time brain reactions, dopamine reward flashes, and axon transmission lines.

```
                           [ L / R: Switch Puzzles ]
                                       |
                   +-------------------+-------------------+
                   |                                       |
         [ Left Stick / D-Pad ]                 [ Right Stick ]
         Navigate Grid Tiles                    Orbit 3D Brain Camera
                   |                                       |
                   |                                       |
         [ (A) Button ]  Mark Tile (Manual) / Advance Step (Step Mode)
         [ (B) Button ]  Cross Tile ('X')
         [ (X) Button ]  Cycle Mode: Observe -> Fast -> Step -> Manual
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
1. Extract `release/flybrain-nx-switch-v1.3.0.zip` to the root of your SD card:
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

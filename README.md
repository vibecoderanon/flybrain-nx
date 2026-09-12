# flybrain-nx // Adult Drosophila Melanogaster Connectome SNN Homebrew

[![Platform: Nintendo Switch](https://img.shields.io/badge/Platform-Nintendo_Switch-E60012?logo=nintendoswitch&logoColor=white)](https://github.com/vibecoderanon/flybrain-nx)
[![Language: C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=cplusplus&logoColor=white)](https://github.com/vibecoderanon/flybrain-nx)
[![Toolchain: devkitA64](https://img.shields.io/badge/Toolchain-devkitA64-E65100)](https://devkitpro.org/)
[![Model: FlyWire 2024](https://img.shields.io/badge/Model-FlyWire_Connectome-00E5FF)](https://flywire.ai/)
[![View App Showcase](https://img.shields.io/badge/Web_Portal-View_App-00E5FF?style=for-the-badge&logo=safari&logoColor=white)](https://vibecoderanon.github.io/#flybrain-nx)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An authentic Nintendo Switch homebrew application executing a real-time **Leaky Integrate-and-Fire (LIF)** spiking neural network simulation of the adult fruit fly (*Drosophila melanogaster*) connectome (**139,255 neurons and 54,498,591 synapses**).

Features an interactive **3D brain point cloud** rendered at 60 FPS, real-time **Joy-Con optogenetic sensory probes**, strict **Title Override enforcement** to guarantee memory safety against Horizon OS firmware updates (Firmware 22.5.0+), and a zero-overhead **$O(1)$ Speed Selector** to switch between raw anatomical fidelity and 1,000 Hz real-time computation.

---

## Quick Navigation
* [Web Portal Showcase](https://vibecoderanon.github.io/#flybrain-nx)
* [Memory Schema & Hardware Budget](#memory-schema--hardware-budget)
* [Zero-Overhead Speed Selector](#zero-overhead-runtime-speed-selector)
* [Joy-Con Optogenetic Controls](#joy-con-optogenetic-controls)
* [Architecture & File Responsibilities](#architecture--file-responsibilities)
* [Building & Console Deployment](#building--console-deployment)
* [Title Override Invariant](#title-override-invariant)
* [Credits & Scientific Acknowledgments](#credits--scientific-acknowledgments)

---

## Memory Schema & Hardware Budget

On the Nintendo Switch (Nvidia Tegra X1, 4 GB LPDDR4), retail applications running under **Title Override** receive an isolated memory pool of $\sim 2.7\text{ GB}$. `flybrain-nx` loads the entire, unpruned FlyWire connectome directly into memory:

| Subsystem / Data Structure | Element Count | Size per Element | Total Memory Footprint | Memory Pool Ratio ($\approx 2.7\text{ GB}$) |
| :--- | :--- | :--- | :--- | :--- |
| **Synapse CSR Table** | 54,498,591 synapses | 8 bytes (`SynapseRecord`) | **$\approx 311.8\text{ MB}$** | $\sim 11.5\%$ |
| **Neuron State & Offsets** | 139,255 neurons | 32 bytes (`NeuronRecord`) | **$\approx 4.45\text{ MB}$** | $< 0.2\%$ |
| **LIF Simulation Dynamic State** | 139,255 states | 16 bytes (`NeuronState`) | **$\approx 2.22\text{ MB}$** | $< 0.1\%$ |
| **Double-Buffered Accumulators**| 139,255 accumulators | 4 bytes (`atomic<float>`) | **$\approx 0.55\text{ MB}$** | $< 0.05\%$ |
| **3D Point Cloud Vertices** | 139,255 vertices | 16 bytes (`BrainVertex`) | **$\approx 2.22\text{ MB}$** | $< 0.1\%$ |
| **Framebuffers (1280x720)** | 2 buffers | RGBA8888 double-buffered | **$\approx 7.37\text{ MB}$** | $< 0.3\%$ |
| **Sensory / Motor LUTs & Heap** | $\sim 7,000$ indices | Metadata & ImGui HUD | **$\approx 28.5\text{ MB}$** | $< 1.0\%$ |
| **TOTAL RUNTIME FOOTPRINT** | | | **$\approx 357.1\text{ MB}$** | **$\approx 13.2\%$** |

---

## Zero-Overhead Runtime Speed Selector

Because biological neural networks have power-law weight distributions, the FlyWire connectome contains millions of single-vesicle contacts ($w = 1$) that neuroscientists routinely classify as anatomical noise. 

In `flybrain-nx`, each neuron's outgoing synapses are pre-sorted descending by synaptic weight in the binary CSR format:
```
[ Strong Synapses (w >= 3) | Moderate Synapses (w == 2) | Noise Contacts (w == 1) ]
```

Switching modes in real-time by pressing **[X]** adjusts only the loop bounds in the inner simulation loop, requiring **zero memory reallocation or reloading**:

```
[Simulation Speed Selector]
  Mode 1: Raw Anatomical Fidelity  (w >= 1) -> All 54.5M synapses active | ~180-250 Hz tick rate
  Mode 2: Real-Time 1kHz Clock    (w >= 2) -> ~16.2M strong connections  | 1,000 Hz real-time compute
  Mode 3: High-Speed Fast-Forward (w >= 3) -> ~9.4M high-weight backbone | 2,000+ Hz throughput
```

---

## Joy-Con Optogenetic Controls

Experience direct sensory-to-motor stimulation across the biological brain lobes:

```
               [ L-Stick: Optic Flow Motion ]        [ R-Stick: 3D Camera Orbit ]
                           \                                  /
                            +--------------------------------+
                            |  (-)               (+) [Exit]  |
                            |   [D-Pad: Zoom]    (X) [Speed] |
                            |                    (A) [Odor]  |
                            +--------------------------------+
                           /                                  \
             [ ZL: Sweet Taste ]                     [ ZR: Bitter Taste ]
```

* **Left Stick:** Optic flow motion vector (drives compound eye ommatidia and Medulla/Lobula motion detectors).
* **Right Stick:** 3D orbit camera (X-axis controls yaw rotation, Y-axis controls pitch).
* **D-Pad Up / Down:** Camera zoom in / zoom out.
* **ZL Trigger:** Sweet taste stimulation (activates proboscis extension and forward approach motor centers).
* **ZR Trigger:** Bitter taste stimulation (activates aversive retreat and stopping motor circuits).
* **(A) Button:** Antennal odor puff (stimulates olfactory receptor neurons in the Antennal Lobe).
* **(R3) Right Stick Click:** Virtual predator looming shadow (triggers bilateral Giant Fiber emergency escape jump).
* **(X) Button:** Cycle simulation speed mode (`Raw` $\rightarrow$ `Real-Time 1kHz` $\rightarrow$ `High-Speed`).
* **(Y) Button:** Toggle Dynamic Synaptic Axon Lines (`ON` / `OFF`) to visualize directional electrical transmission beams between active neurons.
* **(+) Button:** Clean exit to Horizon OS HOME Menu.

---

## Architecture & File Responsibilities

```
flybrain-nx/
├── .github/workflows/
│   └── build.yml               # Containerized devkitA64 CI workflow (devkitpro/devkita64)
├── Makefile                    # devkitA64 standard libnx Nintendo Switch build specification
├── build_release.py            # Standalone SD card release packager (sdmc:/switch/flybrain-nx/)
├── icon.jpg                    # High-resolution 256x256 glowing connectome homebrew icon
├── romfs/
│   └── drosophila_full.bin     # Packed binary CSR connectome (FLYB format, 32B neurons, 8B synapses)
├── src/
│   ├── main.cpp                # Application entry point, main render loop, Title Override guard
│   ├── arena/
│   │   ├── embodied_fly.hpp    # Closed-loop virtual fly body physics & arena simulation
│   │   └── embodied_fly.cpp    # Motor readout to position integration & sensory feedback
│   ├── connectome/
│   │   ├── connectome_format.hpp # Binary FLYB specification, 64B header, 32B neuron, 8B synapse
│   │   ├── connectome_loader.hpp # Memory-mapped loader with synthetic reference fallback
│   │   └── connectome_loader.cpp # Binary parser and neuropil spatial generator
│   ├── graphics/
│   │   ├── brain_renderer.hpp  # 3D point cloud camera, projection, and glow visualizer
│   │   └── brain_renderer.cpp  # Software rasterizer with perspective projection and spike luminance
│   ├── guard/
│   │   ├── applet_guard.hpp    # appletGetAppletType() memory guard
│   │   └── applet_guard.cpp    # Instructional Title Override warning screen (prevents Album OOM)
│   ├── input/
│   │   ├── sensory_probes.hpp  # Joy-Con input mapping to biological sensory modalities
│   │   └── sensory_probes.cpp  # Optogenetic stimulation injectors & camera control polling
│   ├── simulation/
│   │   ├── lif_engine.hpp      # Spiking Leaky Integrate-and-Fire simulation core
│   │   └── lif_engine.cpp      # O(1) speed-bounded synapse propagation and telemetry
│   └── ui/
│       ├── hud.hpp             # On-screen telemetry HUD header
│       └── hud.cpp             # Embedded 8x8 bitmap font renderer & neuropil activity meters
├── tests/
│   ├── test_lif_engine.cpp     # C++ host unit test suite verifying LIF equations and bounds
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
1. Generate or verify the RomFS connectome dataset:
   ```bash
   python3 tools/extract_connectome.py --generate-reference --neurons 10000 --output romfs/drosophila_full.bin
   ```
2. Compile the native Switch `.nro` executable:
   ```bash
   make -j$(nproc)
   ```
3. Package the all-in-one SD card release archive:
   ```bash
   python3 build_release.py
   ```

### SD Card Deployment
1. Copy `release/flybrain-nx-switch-v1.0.0.zip` or extract directly to the root of your SD card:
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

## Title Override Invariant

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

* **FlyWire Consortium & Princeton University:** For the groundbreaking complete adult *Drosophila melanogaster* connectome (Dorkenwald et al., Schlegel et al., *Nature* 2024).
* **Philip Shiu et al.:** For the foundational *Drosophila* Leaky Integrate-and-Fire computational brain model and biological noise thresholding methodology.
* **devkitPro Team:** For `devkitA64`, `libnx`, and the open-source Nintendo Switch homebrew toolchain.
* **vibecoderanon:** Architecture, native C++ SNN engine, 3D point cloud visualizer, and Title Override safeguard.

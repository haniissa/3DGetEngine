> **An interactive, educational turbojet engine simulation built in modern C++ with Vulkan.**

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red)
![CMake](https://img.shields.io/badge/CMake-3.25%2B-green)

----
## Overview 

** 3D Jet Engine Lab** is a real-time, interactive simulation of a turbojet engine's Brayton cycle. It procedurally generates 3D engine geometry, simulates thrmodynamic processes, and visualizees airflow, pressure, and temperature -- all rendered with Vulkan for maximum performance.

### Key Features

| Feature | Description |
|---------|-------------|
|**Procedural 3D Engine** | Lathe-generated compressor, combustor, rurbine, and nozzle | 
|**Interactive Control**| Start/stop, throttle, orbit camera, zoom, pan |
|**Explaoaded View**| Cutaway visulization  of engine interior |
|**Airflow Paricles** | Color-coded particles show temprature changes through stages |
|**Animated T-s Diagram** | Temerature-Entropy plot with live cycle tracking|
|**Live Station Datat**| Real-time P, T, velocity, and density at 6 stations|
|**Guided Chapters**| 5-step educational tour through the Brayton cycle |
|**Fault Challenges** | Diagnostic mode with injectable engine faults |
|**Formula Display**| Key thermodynamic equations with live values|

---

## Architecture 
```
3DJetEngineLab/
├── CMakeLists.txt              # Build configuration (fetches all deps)
├── src/
│   ├── main.cpp                # Entry point
│   ├── App.h / App.cpp          # Main loop orchestrator
│   ├── Renderer.h / Renderer.cpp # Vulkan backend (vk-bootstrap)
│   ├── EngineModel.h / .cpp     # Procedural geometry generator
│   ├── EngineSimulation.h / .cpp # Brayton cycle physics
│   ├── Camera.h / .cpp          # Orbit camera
│   ├── UIManager.h / .cpp       # ImGui panels & diagrams
│   ├── SceneManager.h / .cpp    # Exploded/section view transitions
│   ├── ParticleSystem.h / .cpp  # Airflow visualization
│   └── BraytonCycle.h / .cpp   # Cycle diagram math helper
├── shaders/
│   ├── shader.vert             # Vertex shader (GLSL)
│   └── shader.frag             # Fragment shader (GLSL)
└── README.md                   # This file
```

### Technology Stack
- **C++20** --Modern language features  (concepts, ranges where applicable)
- **Vulkan 1.3** --Low-level GPU graphics API
- **vk-boostrap** --Simplifies Vulkan instance/device/swapchain creation
- **SDL3** --Cross-platform windoing, input, and events 
- **Deat ImGui --Immdiate-mode GUI for panels and diagrams
- **GLM** --Mathematics library (vectors, matrices, transforms)
- **fmt** --Modern formatting and logging
- **CMake 3.25** --Build system with automatic dependency fetching

---
### Build Instructions
### Prerequisites

- **CMake 3.25** or newer
- **C++20 compatible compiler**:
  - MSVC 2019+ (Windows)
  - GCC 11+ (Linux)
  - Clang 14+ (macOS -limited Vulkn support)
- **Vulkan SDK** --Download from [lunarg.com/vulkan-sdk](https://lunarg.com/vulkan-sdk/)
- **Git** --For fetching dependencies

### Step-by-Step Build
```bash
# 1. Clone the repository
git clone <repository-url>
cd 3DJetEngineLab

# 2. Create build directory
mkdir build
cd build

# 3. Configure (CMake fetches SDL3, vk-bootstrap, fmt, ImGui, GLM automatically)
cmake .. -DCMAKE_BUILD_TYPE=Release

# 4. Build (parallel compilation recommended)
cmake --build . --config Release --parallel

# 5. Run
./3DJetEngineLab
```

### Windows (Visual Studio)

```powershell
# Using CMake presets (Visual Studio 2022)
cmake --preset vs2022
# Or manually:
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Linux 
```bash
# Install Vulkan loader (Ununtu/Debian)
sudo apt install libvulkan-dev vulkan-validationlayers-dev

#Build 
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./3DJetEngineLab
```

### macOS (Experimental)
>Note: macOS requires MoltenVK for Vulkan support. Install via Vulkan SDK

```bash
brew install cmake
export VULKAN_SDK=/path/to/vulkan-sdk
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
./3DJetEngineLab
```
---
## Controls
|key/Input | Action|
|----------|-------|
|`↑` / `↓` |Increase / Decrease Throttle |
|`E` | Toggle Exploded View |
|`S` | Toggle Section View |
|`A` | Toggle Airflow Particles  |
|`C` | Next Guided Chapter |
|`F` | Inject Fault (Challenge Mode) |
|`R` | Toggle Auto-Rotate Camera |
|`Esc`| Quit |
| **Left Mouse + Drag**| Orbit Camera |
|**Scroll Wheel** | Zoom In / Out |
|**Right Mouse + Drag** | Pan Camera |

---

## Physicas Model 
The simulation implements the  **ideal Brayton cycle** with these processes: 

1. **Intake (0-1)** Isentropic deceleration -- `T₀/T = 1 + ((y-1)/2)M²`
2. **CVompression (1-2)**: Isentropic compression -- `T₂/T₁ = (P₂/P₁)^((γ-1)/γ)`
3. **Combustion (2-3)**: Isobaric heat addition -- `Qᵢₙ = cₚ·(T₃ - T₂)`
4. **Expansion (3-4)**: Isentropic expansion -- work drives compressor
5. **Exhaust (4-5)**: isentropic acceleration -- `vₑₓᵢₜ = √(2·cₚ·(T₄ - T₅))`

** Thrust**  `F = ṁ·(vₑₓᵢₜ - vᵢₙₗₑₜ)`
** Thermal Efficiency** `η = 1 - (T₁/T₃)`


---
## Portfolio Notes 
- **Modern C++**: Smart pointers, RAII, constexpr, structured bindings
- **Graphics Programming**: Vulkan API, shader pipelines, uniform buffers
- *Procedural Generation**: Mathematical mesh construction (lath surfaces) 
- **Real-time Simulation**: Physics interation with frame-rate independnce
- **UI/UX Design**: ImGui interation, data visulization, responsive layout
- **Software Architecture**: clean separation of concerns, dependency injection
- **Build Engineering**: CMake with Fetchcontent, cross-platform support

---

## License 
This project is created for portfolio and educational purposes.

## Acknowledgements 
- [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) — Charles Giessen
- [Dear ImGui](https://github.com/ocornut/imgui) — Omar Cornut
- [SDL3](https://github.com/libsdl-org/SDL) — SDL Community
- [fmt](https://github.com/fmtlib/fmt) — Victor Zverovich
- [GLM](https://github.com/g-truc/glm) — Christophe Riccio

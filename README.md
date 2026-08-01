# FluidSim-1

An interactive 2D fluid-particle sandbox written in **C++20** with **SFML 3**. It uses weakly-compressible Smoothed Particle Hydrodynamics (SPH) to simulate density, pressure, viscosity, gravity, collisions, and solid platforms in real time.

## Features

- SPH-based fluid motion with physically motivated density, pressure, and viscosity forces
- Interactive fluid injection with a resizable particle brush
- Random floating solid platforms with particle collision response
- Adjustable gravity and viscosity while the simulation runs
- Uniform spatial hashing for efficient local neighbor searches
- Batched GPU rendering: all particles render in a single draw call
- CPU parallelism for large scenes
- Automatic removal of settled floor  particles to preserve performance

## Interface

The application has a live dashboard showing active particles, gravity, viscosity, and smoothed FPS. The tank includes a subtle grid for spatial reference, and the cursor previews the exact fluid block that will be spawned. The UI uses the standard macOS Arial font when it is available; the simulation remains fully functional without it.

## Controls

| Input | Action |
| --- | --- |
| Left click | Spawn fluid using the current brush size |
| Left / Right arrow | Decrease / increase brush width |
| Down / Up arrow | Decrease / increase brush height |
| W / S | Increase / decrease gravity |
| A / D | Decrease / increase viscosity |
| Space | Pause or resume |
| R | Reset fluid and generate new platforms |

The window title displays the active particle count, gravity, viscosity, and current brush dimensions.

## Physics model

For each particle, local density is estimated from nearby particles using the Poly6 smoothing kernel:

\[
\rho_i = \sum_j m_j W_{poly6}(\lVert \mathbf{r}_i - \mathbf{r}_j \rVert, h)
\]

Pressure is calculated with a weakly compressible equation of state:

\[
p_i = k\max(0, \rho_i - \rho_0)
\]

The simulation applies pressure and viscosity forces using the Spiky-gradient and viscosity-Laplacian kernels, then integrates motion with a fixed timestep:

\[
\mathbf{v}_{t + \Delta t} = \mathbf{v}_t + \frac{\mathbf{F}}{\rho}\Delta t
\qquad
\mathbf{x}_{t + \Delta t} = \mathbf{x}_t + \mathbf{v}_{t + \Delta t}\Delta t
\]

Particle velocity is reflected and damped on tank walls and solid-platform surfaces.

## Performance design

The world is divided into grid cells sized to the smoothing radius. A particle only checks its own cell and the eight surrounding cells, avoiding an all-pairs particle search.

At 2,000 or more active particles, density, force, and integration stages are split across available CPU cores. Smaller scenes remain single-threaded because the cost of creating worker threads would outweigh the benefit. Rendering stays GPU-batched in all cases.

Particles that remain still on the tank floor for 0.45 seconds are discarded, since they no longer contribute to visible motion. Fluid injection is capped at 6,000 active particles.

## Requirements

- CMake 3.20+
- A C++20 compiler
- SFML 3.0+

### macOS setup

```sh
brew install sfml
```

## Build and run

```sh
git clone https://github.com/peprick/FluidSim-1.git
cd FluidSim-1
cmake -S . -B build
cmake --build build
./build/FluidSim
```

## Project structure

```text
FluidSim-1/
├── include/FluidSimulation.hpp  # Simulation data types and public API
├── src/FluidSimulation.cpp      # SPH physics, grid, collisions, rendering
├── src/main.cpp                 # SFML window, input, and scene drawing
└── CMakeLists.txt               # Build configuration
```

## License

Distributed under the [Apache License 2.0](LICENSE).

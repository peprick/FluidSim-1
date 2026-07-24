# FluidSim-1

An interactive 2D fluid-like particle sandbox written in C++20 using weakly-compressible Smoothed Particle Hydrodynamics (SPH).

## Physics

Each particle estimates local density using the Poly6 smoothing kernel, calculates pressure using an equation of state, then combines pressure, viscosity, gravity, and boundary forces. A uniform spatial hash limits interactions to nearby particles.

For sustained performance, particles that have rested on the floor for 0.45 seconds are removed from the simulation. Fluid injection is capped at 6,000 active particles.

Rendering is batched into a single GPU draw call. When the active count reaches 2,000, density, force, and integration stages are split across the available CPU cores; smaller scenes stay single-threaded to avoid thread-startup overhead.

## Controls

| Input | Action |
| --- | --- |
| Left click | Add a block of fluid at the current brush size |
| Space | Pause / resume |
| R | Reset the simulation |
| Arrow keys | Resize the fluid brush: left/right is width; up/down is height |
| W / S | Increase / decrease gravity |
| A / D | Decrease / increase viscosity |

## Build

Install SFML 3.0 or newer, then configure and build:

```sh
cmake -S . -B build
cmake --build build
./build/FluidSim
```

On macOS with Homebrew, SFML can be installed with `brew install sfml`.

# Sierpinski Visualizer

A live Sierpinski triangle drawn with the **chaos game**, written in C++ with [raylib](https://www.raylib.com/).
Pick a starting point, watch the first few jumps step by step, then watch millions of points per second fill in the fractal.

## The idea

1. Take three corners of a triangle and any starting point.
2. Pick a corner at random and jump halfway towards it.
3. Plot the new point, then repeat.

Whatever the starting point is, the plotted points settle onto the Sierpinski triangle.
Each jump halves the distance to a corner, so any starting error shrinks by half every step and is smaller than a pixel after about 20 steps.
The first 20 points are skipped in this implementation to avoid errors.

## Features

- **Live accumulation:** points are drawn into an offscreen render texture that is never cleared, so the fractal builds up on screen.
- **Choose your own start:** left-click anywhere, even far outside the triangle, and watch it converge.
- **Step-by-step demo:** the first 20 jumps are shown slowly to visualize the creation of the fractal.
- **Adaptive step cap:** the program measures how long one step takes and limits the steps per frame to fit a time budget, so the frame rate holds near its target on weaker hardware.
- **Live stats:** FPS, total points, and points per second.

## Controls

| Input | Action |
|---|---|
| Left click | Choose the starting point and begin |
| `Space` | Pause / resume |
| `R` | Reset and choose a new start |
| `B` | Benchmark Mode |

## Build

Requirements: a C++17 (or newer) compiler, CMake 3.14+, and git. Raylib is downloaded and built automatically with CMake's `FetchContent`.

```
git clone https://github.com/SayanNnd/Sierpinski-Visualizer
cd Sierpinski-Visualizer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Always benchmark with a **Release** build. The Debug build has no optimisation and gives misleadingly low numbers.

Developed on Windows with CLion and MinGW.

## Benchmarks

Measured on a laptop with an RTX 5050, plugged in, running in DGPU mode, Release build, single-threaded point generation.

| Setting | FPS | Points / second |
|---|---|---|
| 100,000 steps per frame, fixed | ~119 | 11.77M (limited by the 120 FPS cap) |
| 200,000 steps per frame, fixed | ~80 | 16.57M (CPU-limited) |
| Adaptive cap, 70% frame budget | ~118 | 11.48M |
| Adaptive cap, 90% frame budget | ~116 | 15.14M |

Notes:

- At a fixed 100,000 steps the rate is just `steps × FPS`. The frame cap sets it, not the CPU.
- The 16.57M figure is the single-thread ceiling of the current design. At 300,000 steps per frame the frame rate collapses and the rate drops.
- With an adaptive budget the throughput is roughly `budget fraction ÷ cost per step`, so a larger budget means more points per second and less spare time per frame.
- The generation loop runs on the CPU. The GPU only displays the finished image. (--Key Issue to work on for Next Version)

## Roadmap

- Multi-threaded point generation, with a comparison of 1 thread against N threads
- A separate OpenGL rendering experiment
- More shapes and jump ratios (square, pentagon, Barnsley fern)
- A CUDA version and a CPU / GPU comparison
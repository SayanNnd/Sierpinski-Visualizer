# Sierpinski Visualizer

A live Sierpinski triangle drawn with the **chaos game**, written in C++ with [raylib](https://www.raylib.com/).
Pick a starting point, watch the first few jumps step by step, then watch up to a billion points per second fill in the fractal.

## The idea

1. Take three corners of a triangle and any starting point.
2. Pick a corner at random and jump halfway towards it.
3. Plot the new point, then repeat.

Whatever the starting point is, the plotted points settle onto the Sierpinski triangle.
Each jump halves the distance to a corner, so any starting error shrinks by half every step and is smaller than a pixel after about 20 steps.
The first 20 points are skipped in this implementation to avoid errors.

## Features

- **Live accumulation:** points are written into a CPU-side pixel array that is never cleared, and the array is uploaded to a texture once per frame, so the fractal builds up on screen.
- **Multi-threaded point generation:** worker threads each run their own chaos game on private data, and the results are merged once per frame.
- **Step-by-step demo:** the first 20 jumps are shown slowly to visualize the creation of the fractal, then the rate of points ramps up smoothly to full speed.
- **Adaptive step cap:** the program measures how long one step takes and limits the steps per frame to fit a time budget (after subtracting the texture upload time), so the frame rate holds near its target on weaker hardware.
- **Fast random numbers:** a small xorshift64 generator picks the corners instead of raylib's `GetRandomValue`.
- **Live stats:** FPS, total points, and points per second. Benchmark mode also shows the per-frame timing of the threaded section.

## Controls

| Input | Action |
|---|---|
| Left click | Choose the starting point and begin |
| `Space` | Pause / resume |
| `R` | Reset and choose a new start |
| `B` | Benchmark Mode |

## Build

Requirements: a C++20 compiler (GCC, Clang or MSVC), CMake 3.14+, and git. Raylib is downloaded and built automatically with CMake's `FetchContent`.

```
git clone https://github.com/SayanNnd/Sierpinski-Visualizer
cd Sierpinski-Visualizer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Always benchmark with a **Release** build. The Debug build has no optimisation and gives misleadingly low numbers.

The number of threads is set by `thread_count` at the top of `main.cpp`. It defaults to `hardware_concurrency() / 4`, which is 4 on the development machine.
Developed on Windows with CLion and MinGW.

## Usage

Run the executable (with the Visual Studio generator it is in `build\Release\`, otherwise in `build/`):

```
Sierpinski_Visualizer
Sierpinski_Visualizer --threads 4
```

`--threads N` (or `-t N`) sets the number of threads used for point generation. N must be between 1 and the number of logical processors. Without it, the default is `hardware_concurrency() / 2` 

## Benchmarks

Measured on a laptop with an AMD Ryzen 7 260 (8 cores / 16 threads) and a Mobile RTX 5050, plugged in, running in DGPU mode, Release build, 1080x1080 window, adaptive cap at 90% of the frame budget.
These are single runs on one machine, so treat them as rough figures.

**Version 1: points drawn with `DrawPixelV` into a render texture** (single-threaded)

| Setting | FPS | Points / second |
|---|---|---|
| 100,000 steps per frame, fixed | ~119 | 11.77M (limited by the 120 FPS cap) |
| 200,000 steps per frame, fixed | ~80 | 16.57M (CPU-limited) |
| Adaptive cap, 70% frame budget | ~118 | 11.48M |
| Adaptive cap, 90% frame budget | ~116 | 15.14M |

**Version 2: points written into a CPU pixel array, uploaded once per frame** (single-threaded)

| Random number generator | FPS | Points / second |
|---|---|---|
| raylib `GetRandomValue` | ~119 | ~180M |
| xorshift64 | ~119 | ~420M |

**Version 3: multi-threaded generation** (xorshift64, sliced merge)

| Threads | Peak points / second |
|---|---|
| 1 | ~400-430M |
| 2 | ~650M |
| 4 | ~1.3B |
| 8 | ~1.5B |

### Notes

- At a fixed 100,000 steps the rate is just `steps × FPS`. The frame cap sets it, not the CPU.
- With an adaptive budget the throughput is roughly `budget fraction ÷ cost per step`, so a larger budget means more points per second and less spare time per frame.
- Version 2 is about 12x faster than version 1 at the same 90% budget. Most of the old cost was in raylib's drawing path (every `DrawPixelV` call became vertices in a batch buffer), not in the random number and midpoint math. Swapping `GetRandomValue` for xorshift64 more than doubled the rate again. The cost per step went from roughly 60 ns to roughly 5 ns to roughly 2 ns (estimated from the measured rates and the frame budget).
- **Points per second counts steps, not visual progress.** The 1080x1080 triangle fills in within about a second, and after that nearly every new point lands on a pixel that is already red. Threads raise the step rate, but the picture looks the same. The value of this version is the parallel-programming baseline for the GPU versions.
- **The merge cost grows with the number of workers**, because every extra worker adds another full-size flag buffer to scan.
- The point generation is still on the CPU. The GPU only displays the finished image.


## Roadmap

- Dynamic work distribution (threads take chunks from a shared counter) so fast threads do not wait for slow ones
- A separate OpenGL rendering experiment: a fragment shader deciding each pixel on the GPU, with zoom and pan
- More shapes and jump ratios (square, pentagon, Barnsley fern)
- A CUDA version and a CPU / GPU comparison

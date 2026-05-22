# THRΞSH

A modern game engine. Vulkan renderer, flecs ECS, slang shaders, C++26. Very early — the API is going to move around, see [ROADMAP.md](ROADMAP.md) for what's coming.

## Before you build

You'll need these on your system — neither is vendored:

- **Vulkan SDK** — install from [vulkan.lunarg.com](https://vulkan.lunarg.com/) (or your distro's package). `vulkaninfo` should run.
- **slangc** — the Slang shader compiler, on your `PATH`. Grab a release from [shader-slang/slang](https://github.com/shader-slang/slang/releases).

Everything else (SDL3, flecs, glaze, imgui, …) is pulled in via CPM at configure time.

You also need CMake ≥ 4.2 and a compiler that handles C++26 (recent GCC, Clang, or MSVC).

## Build & run

```sh
cmake -B build -S .
cmake --build build
./build/examples/demo-app/demo
```

That runs the demo app, which is the best place to start.

## Learning the engine

Read [`examples/demo-app/`](examples/demo-app/). It's a small game-side `App` linking against the `Thresh` library and exercises most of the public API — windowing, scene setup, asset loading, the render loop. New examples will land in `examples/` as features come online.

## Status

Pre-alpha. Things break, things move, nothing about the API is stable. If you're poking at this and something seems wrong, it probably is — check the roadmap or open an issue.

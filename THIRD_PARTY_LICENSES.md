# Third-Party Licenses

THRESH itself is MPL 2.0 (see [LICENSE](LICENSE)). It pulls in the libraries below at build time via CPM. All of them are permissively licensed — none impose copyleft obligations on the engine or on games built with it — but if you redistribute a compiled binary that links them, you need to ship their license texts alongside it. This file is the index for that.

The actual license text for each library lives in its own source tree (under `cmake-build-*/_deps/<name>-src/` after a configure, or in the upstream repo). If you cut a release, vendor those license files into your distribution.

| Library | Used for | License | Upstream |
|---|---|---|---|
| SDL3 | Windowing, input, controller support | zlib | https://github.com/libsdl-org/SDL |
| GLM | Math (vectors, matrices, quaternions) | MIT / Happy Bunny | https://github.com/g-truc/glm |
| flecs | ECS | MIT | https://github.com/SanderMertens/flecs |
| PhysicsFS | Virtual filesystem (mounting folders + archives) | zlib | https://github.com/icculus/physfs |
| stb | `stb_image` for texture decoding | Public Domain / MIT (dual) | https://github.com/nothings/stb |
| KTX-Software | KTX2 + Basis Universal texture loading | Apache 2.0 (+ component licenses, see upstream LICENSE.md) | https://github.com/KhronosGroup/KTX-Software |
| Dear ImGui | Debug UI | MIT | https://github.com/ocornut/imgui |
| glaze | JSON (de)serialisation | MIT | https://github.com/stephenberry/glaze |
| Vulkan-Headers / Vulkan-Loader | Vulkan API access | Apache 2.0 | https://github.com/KhronosGroup/Vulkan-Headers |

`slangc` is a separate tool that THRESH *invokes* at build time to compile shaders — its license (Apache 2.0 with LLVM exception) only applies to the compiler itself, not to the engine binary

## Compliance checklist for binary releases

- Ship a `licenses/` folder (or equivalent) containing each library's LICENSE/COPYING file verbatim.
- Don't strip the copyright headers from any vendored source files.
- For Apache 2.0 dependencies (KTX, Vulkan), preserve any upstream `NOTICE` files.
- KTX-Software bundles components under additional licenses (Basis Universal, etc.); copy its full `LICENSE.md` rather than abbreviating.
- Mention THRESH itself under MPL 2.0 in your credits — the engine's `LICENSE` file qualifies.

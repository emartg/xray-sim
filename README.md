# Projectional Radiography Simulator

Desktop application that uses a custom OpenGL graphics engine, the gVirtualXRay library and ImGui, among other tools and libraries.
It allows radiology professionals and students to test X-ray scanning and imaging in a virtual environment.

## Repository Structure

- `engine/`: Git submodule with the [OpenGL graphics engine](https://github.com/emartg/opengl-graphics-engine) (`Engine::Core` and `Engine::Platform`).
- `gvirtual-xray/`: Git submodule with the [fork of gVirtualXRay](https://github.com/emartg/gvirtual-xray) that computes the X-ray images.
- `src/`: the code of the simulator, which connects both: the application (`src/app`) and the library that adapts gVirtualXRay (`src/xray`).
- `tests/`: the tests of the simulator's libraries.
- `cmake/`: the vcpkg triplets used by the Windows presets.
- `docs/`: documentation (the build instructions and the credits of the third-party libraries and resources).

The simulator depends on the engine and on gVirtualXRay. The engine never depends on, or mentions, the simulator or gVirtualXRay.

## Getting the Sources

gVirtualXRay stores large assets with Git LFS, so [install Git LFS](https://git-lfs.com) before cloning:

```
git lfs install
git clone --recurse-submodules https://github.com/emartg/xray-sim.git
```

If the repository was cloned without `--recurse-submodules`, run `git submodule update --init --recursive`. The engine's resources and the LFS assets are large (hundreds of megabytes), so the first clone takes a while.

## Status

Early stage. The repository builds a minimal `XRSim` application on top of the engine (a window with a placeholder panel), and the core library of gVirtualXRay is part of the build, with a small adapter library and its test (on Linux, and on Windows with MinGW; not yet with MSVC). The next step is a prototype that connects the two: a scene of simple shapes drawn by the engine, shown through the X-ray image that gVirtualXRay computes, either beside the normal view or alone. The simulator itself is not written yet. See [docs/build_instructions.md](docs/build_instructions.md) to build and run it.

## Assets and Credits

The third-party libraries and resources of the simulator are credited, with their authors, sources, and licenses, in [docs/sources.md](docs/sources.md). The engine and the gVirtualXRay fork have their own files: [engine/docs/sources.md](engine/docs/sources.md) and [gvirtual-xray/doc/SOURCES.md](gvirtual-xray/doc/SOURCES.md).

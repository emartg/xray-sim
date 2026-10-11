# Projectional Radiography Simulator

Desktop application that uses a custom OpenGL graphics engine, the gVirtualXRay library and ImGui, among other tools and libraries.
It allows radiology professionals and students to test X-ray scanning and imaging in a virtual environment.

## Repository Structure

- `engine/`: Git submodule with the [OpenGL graphics engine](https://github.com/emartg/opengl-graphics-engine) (`Engine::Core` and `Engine::Platform`).
- `gvirtual-xray/`: Git submodule with the [fork of gVirtualXRay](https://github.com/emartg/gvirtual-xray) that computes the X-ray images.
- `src/`: the code of the simulator, which connects both (not written yet).
- `docs/`: documentation (for now, the credits of the third-party libraries and resources).

The simulator depends on the engine and on gVirtualXRay. The engine never depends on, or mentions, the simulator or gVirtualXRay.

## Getting the Sources

gVirtualXRay stores large assets with Git LFS, so [install Git LFS](https://git-lfs.com) before cloning:

```
git lfs install
git clone --recurse-submodules https://github.com/emartg/xray-sim.git
```

If the repository was cloned without `--recurse-submodules`, run `git submodule update --init --recursive`. The engine's resources and the LFS assets are large (hundreds of megabytes), so the first clone takes a while.

## Status

Early stage: the layout of the repository is set up, and the simulator itself is not written yet. The build files will be added in the next steps.

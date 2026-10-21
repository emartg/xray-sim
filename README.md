# Projectional Radiography Simulator

Desktop application that uses a custom OpenGL graphics engine, the gVirtualXRay library and ImGui, among other tools and libraries.
It allows radiology professionals and students to test X-ray scanning and imaging in a virtual environment.

## Repository Structure

- `engine/`: Git submodule with the [OpenGL graphics engine](https://github.com/emartg/opengl-graphics-engine) (`Engine::Core` and `Engine::Platform`).
- `gvirtual-xray/`: Git submodule with the [fork of gVirtualXRay](https://github.com/emartg/gvirtual-xray) that computes the X-ray images.
- `src/`: the code of the simulator, which connects both: the application (`src/app`), the description of the scenes, the materials, and the X-ray setup (`src/scene`), the conversion of an X-ray image to grey values (`src/display`), and the library that adapts gVirtualXRay (`src/xray`).
- `tests/`: the tests of the simulator's libraries and of its build.
- `cmake/`: the vcpkg triplets used by the Windows presets, and the check that the submodules are at the commits that the repository records.
- `CHANGELOG.md`: the changes of the project, version by version.
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

Early prototype. The simulator is built on the engine and on gVirtualXRay, and it already connects them: it shows a scene of simple shapes (a sphere of water, a rotated cube of aluminium, a cylinder of bone, and a small sphere of iron) drawn by the engine, and the X-ray image of the same scene, which gVirtualXRay computes. The image is shown in a window beside the normal view, or as the whole window. A second window sets the energy of the beam, the distances of the source and of the detector to the scene, and the number of pixels and the pixel size of the detector, and the image is computed again when a value changes. The scale of the image can be linear or logarithmic, and negative.

The simulator is built and tested on Linux (GCC and Clang) and on Windows (MinGW; with MSVC the application builds without the X-ray part for now), and CMake checks that the two submodules are at the commits that the repository records. Next are the choice of the material of each object, the window and level of the image, and a first pre-release (v0.1.0). The medical features (exam types, patients, exposure, image quality, dose, and exercises) are not designed yet. See [docs/build_instructions.md](docs/build_instructions.md) to build and run it.

## Assets and Credits

The third-party libraries and resources of the simulator are credited, with their authors, sources, and licenses, in [docs/sources.md](docs/sources.md). The engine and the gVirtualXRay fork have their own files: [engine/docs/sources.md](engine/docs/sources.md) and [gvirtual-xray/doc/SOURCES.md](gvirtual-xray/doc/SOURCES.md).

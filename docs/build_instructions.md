# Build Instructions

This document explains how to build and run the simulator (`XRSim`). The simulator builds the engine as a
subdirectory, so the details of the prerequisites, the dependencies of the engine, and the troubleshooting of
the platforms are in the [engine's build instructions](../engine/docs/build_instructions.md); this document
only covers what is specific to this repository. The simulator also builds the core library of the
gVirtualXRay fork (the X-ray part, option `XRSIM_BUILD_XRAY`, on by default with every compiler: GCC, Clang, MinGW, and MSVC). On
Linux its dependencies are the system packages, and on Windows they are built by vcpkg, like the engine's.

## Prerequisites

- **Operating System:** Windows or Linux. macOS is not supported, since it only provides OpenGL up to 4.1.
- **GPU and drivers:** OpenGL 4.5 core profile support (a recent Mesa on Linux is enough, including the `llvmpipe` software renderer).
- **Compiler:** MSVC from Visual Studio 2022/2026, or GCC from a MinGW installation on Windows; GCC or Clang on Linux. The code is built as C++23, the standard of gVirtualXRay and of the Engine. The continuous integration uses GCC 13.3 and Clang 18.1 (Ubuntu 24.04), MSVC 14.51 (Visual Studio 2026) with Ninja, and MinGW GCC 15.2.
- **CMake:** 3.26 or newer (the minimum of gVirtualXRay), which also covers the presets in `CMakePresets.json`.
- **vcpkg (Windows):** a Git clone of [vcpkg](https://github.com/microsoft/vcpkg), with the `VCPKG_ROOT` environment variable pointing to it.
- **Git and Git LFS:** to clone the repository and its submodules (the configuration of gVirtualXRay checks that Git LFS is installed).

## Getting the Sources

The engine and gVirtualXRay are Git submodules, so clone the repository with its submodules (see the [README](../README.md)), or fetch them afterwards:

```
git submodule update --init engine gvirtual-xray
```

CMake stops with a message if the engine, or gVirtualXRay when the X-ray part is on, is missing. The build does not need the Git LFS files of gVirtualXRay (data and assets): set the environment variable `GIT_LFS_SKIP_SMUDGE=1` before fetching the submodule to skip them.

## The Submodules Must Be at the Recorded Commits

The repository records one commit for each submodule (the pin), but the build uses the contents of the folders `engine/` and `gvirtual-xray/`, whatever commit is checked out in them. So that what is built is what the repository says, CMake compares, when it configures, the commit checked out in each folder (gVirtualXRay only when the X-ray part is on) with the one that the index of the repository records for it (`git ls-files -s <folder>`), and stops with a message that names the folder, both commits, and the two ways out if they differ (the message is expected whenever a folder is on one of its branches because you work on it; the way out 1 also says how to put the folder back on its branch afterwards; see Troubleshooting):

- Go back to the recorded commit: `git submodule update engine` (commit or discard the work in the folder first: Git refuses to overwrite it).
- Move the pin on purpose: `git add engine` and commit it. The index is compared, not the last commit, so a new pin can be tried after `git add` and before the commit.

To configure anyway, for example to try another version without recording it, add `-DXRSIM_ALLOW_UNPINNED_SUBMODULES=ON` (off by default): the stop becomes a warning with the same message. The check compares commits, not the files, so a folder with uncommitted work at the recorded commit passes. It is skipped, with a status message, when it cannot be made: Git is not available, the sources are not a Git checkout (for example, a source archive), a submodule was not fetched, or the repository records no commit for it.

If a submodule folder is also used to work on that submodule, put it at the recorded commit before building the simulator (`git -C engine status --short` must print nothing first, and then `git -C engine switch --detach <commit>`), and go back to the work afterwards.

## Dependencies

The dependencies are the ones of the engine, declared in the `vcpkg.json` manifest of this repository (vcpkg manifest mode), plus the ones of gVirtualXRay:

- **Windows (every preset):** vcpkg builds and installs them into the build directory during the first configuration, for the triplet of the preset (`x64-windows-xray` with MSVC and `x64-mingw-dynamic-xray` with MinGW). These triplets are in `cmake/vcpkg_triplets` and are the ones of vcpkg (`x64-windows` and `x64-mingw-dynamic`) with GLEW as a static library, which gVirtualXRay requires; the MinGW one also keeps the `PATH` out of the hash (ABI) of the packages: vcpkg needs the `PATH` to find the compiler, but with the triplet of vcpkg any difference in it (another terminal, VS Code started from another shell, a runner of the continuous integration) changes the hash of every package, and vcpkg builds them all again instead of restoring them. The first configuration takes a long time (the first one after this change builds the MinGW packages once more), and vcpkg stores the built packages in its binary cache for the next ones.
- **Linux:** the system packages. On Ubuntu:

```
sudo apt-get install ninja-build libglfw3-dev libassimp-dev libglm-dev libgl-dev \
  libglu1-mesa-dev libegl-dev libglew-dev libtiff-dev libfreetype-dev zlib1g-dev libboost-dev libpng-dev libbz2-dev
```

gVirtualXRay is built with these system packages and with its own copy of xraylib. A missing package is not an error: gVirtualXRay would build that dependency from sources instead (and download it), so check that no line of the configuration log starts with `-- Build` followed by the name of a library. Pass `-DXRSIM_BUILD_XRAY=OFF` to build only the engine and the application.

The dependencies of the engine can change when the pinned version of the engine changes (the manifest and the package list above are kept in sync with the pinned version).

## Building and Running

Use one of the presets of `CMakePresets.json`. The names are the same as the engine's: `ninja-msvc-debug`, `ninja-msvc-release`, `vs2022-msvc-*`, `vs2026-msvc-*`, `mingw-gcc-vcpkg-*` (Windows), and `ninja-gcc-*`, `ninja-clang-*` (Linux). The Ninja presets with MSVC must run in an x64 Visual Studio Developer PowerShell (see the engine's instructions).

```powershell
# Windows (Ninja and MSVC, in an x64 developer environment)
cmake --preset ninja-msvc-debug
cmake --build --preset ninja-msvc-debug
.\out\build\ninja-msvc-debug\bin\XRSim.exe
```

```
# Linux
cmake --preset ninja-gcc-debug
cmake --build --preset ninja-gcc-debug
./out/build/ninja-gcc-debug/bin/XRSim
```

The build presets build every target (the application, the libraries, and the tests when they are on). `XRSim --frames N` renders `N` frames and exits (it is used by the automated tests). The executable is at `bin/` in the build directory (`bin/Debug/` or `bin/Release/` with the Visual Studio generators).

## Tests

The tests are built with `-DXRSIM_BUILD_TESTS=ON` (off by default). The first three are labelled `smoke`:

- `XRSim_Smoke_Test` renders 60 frames, and fails if the exit code is not zero or if a line contains `[ERROR`. It needs an OpenGL 4.5 display (on Linux without one, run it with `xvfb-run -a`, which needs the `xvfb` and `libgl1-mesa-dri` packages).
- `XRSim_Invalid_Argument_Test` checks that an invalid argument is rejected. It needs no display.
- `XRSim_Xray_Self_Test` (only with the X-ray part) runs `XRSim --frames 3 --xray-self-test`: in the first frame, on the OpenGL context of the Engine's window, it computes the X-ray image of a cube of water of 10 cm with a beam of 80 keV, and checks that the cube transmits the beam as the Beer-Lambert law predicts (within 3 percent), and that the state of OpenGL is the same before and after the computation. It needs an OpenGL 4.5 display, like the smoke test.
- `XRSim_Xray_View_Test` (only with the X-ray part) runs `XRSim --frames 3 --xray-view-test`: in the first frame the simulator computes the X-ray image of the default scene, and checks that the corners of the image are not attenuated, that the beam is attenuated where the centre of each object projects on the detector, and that the iron sphere attenuates more than the water sphere; then it changes the setup (a lower energy must attenuate more, and another geometry and detector must give a valid image of the new size) and goes back to the first setup. It needs an OpenGL 4.5 display too. The smoke test shows the image as well, without checking it.
- `Submodule_Pin_Guard_Test` (label `unit`) checks the pin guard with small Git repositories that it creates in the build directory: the folder at the recorded commit passes; at another commit the configuration stops with the message that names the folder, both commits, how to go back to the branch afterwards, and the two ways out; the option turns the stop into a warning; a new pin in the index passes; and the check is skipped without Git, without a Git checkout, with a folder that was not fetched, and with no commit recorded. It needs Git (it is skipped without it) and no display.
- `Scene_Test` (label `unit`) checks the scene library without an OpenGL context: the shapes (closed surfaces, outward triangles, and the volume that each one should enclose), the table of materials, and that the default scene gives X-ray meshes in millimetres with the transform of each object applied.
- `Xray_Setup_Test` (label `unit`) checks the setup of the source and the detector (the default values, the conversion from centimetres to the millimetres of the library, the magnification, and the limits of every value) without an OpenGL context.
- `Xray_Display_Test` (label `unit`) checks the conversion of an X-ray image to grey values (the reference, the transmission, the linear and logarithmic scales, and the negative), without an OpenGL context.
- `Gvxr_Adapter_Test` (label `unit`, only with the X-ray part) checks that the simulator can use gVirtualXRay: the version is reported and the library creates a mixture of chemical elements, without an OpenGL context.

```
cmake --preset ninja-gcc-debug -DXRSIM_BUILD_TESTS=ON
cmake --build --preset ninja-gcc-debug
ctest --test-dir out/build/ninja-gcc-debug --output-on-failure
```

## Code Formatting

The code of `src/` is formatted with clang-format 23.1.1, using the `.clang-format` of the repository. The submodules are excluded (`.clang-format-ignore`). To check the formatting:

```
git ls-files '*.c' '*.cpp' '*.h' '*.hpp' | xargs clang-format --style=file --dry-run --Werror
```

## Continuous Integration

The workflow `.github/workflows/ci.yml` runs on every push to `main` and on every pull request:

- **Format:** the clang-format check above.
- **Linux:** the `ninja-gcc-debug` and `ninja-clang-release` presets, with the system packages; it builds and runs the tests under a virtual display (Xvfb) with Mesa's software OpenGL 4.5 renderer.
- **Windows:** the `ninja-msvc-debug` and `mingw-gcc-vcpkg-release` presets, with the dependencies built by vcpkg (stored in a binary cache, with the triplets `x64-windows-xray-ci` and `x64-mingw-dynamic-xray-ci` of `cmake/vcpkg_triplets`, which leave the compiler out of the ABI hash of the packages so that a new build of the compiler in the runner image does not discard the cache, and the host triplet `x64-windows-ci`, which does the same for the tools that vcpkg builds for itself; a compiler upgrade does not rebuild a package in the CI, because the key of the cache has the versions of the compiler, of CMake, and of PowerShell (the hash of every package has them); the summary of each job shows how many packages vcpkg restored and the entries of the hash of `zlib` and `vcpkg-cmake`: if the cache does not restore, the entry that differs between two runs is the cause); it builds and runs only the tests that need no display (the invalid argument test, the unit tests of the scene, the setup, the display, and the pin guard, and the adapter test, because the X-ray part is built in every job), since the runners do not support OpenGL 4.5.

## Troubleshooting

### The pin guard stops the configuration

CMake stops with `The folder 'engine' is not at the commit that this repository records for it` (or `'gvirtual-xray'`). The folder is checked out at another commit than the one that the repository records, most often because it is on its own `main` while you work on it, and the build uses the contents of the folder, not the recorded commit. The message is expected in that case. From the root of the repository, in PowerShell, put the folders at the recorded commits (taken from Git, not from a hash written in a text), delete the build folder, and configure again:

```powershell
git -C engine status --short
git -C gvirtual-xray status --short
git -C engine switch --detach (git rev-parse :engine)
git -C gvirtual-xray switch --detach (git rev-parse :gvirtual-xray)
git submodule status
```

The first two commands must print nothing (otherwise commit or stash the work in that folder first: Git refuses to switch over it), and `git submodule status` must show no `+` at the start of a line. Build, test, and run while the folders are detached: the program reads the shaders and the resources of the Engine from `engine/` every time it runs, so an Engine at another commit crashes it at start-up. Afterwards put the folders back on their branches:

```powershell
git -C engine switch main
git -C engine pull
git -C gvirtual-xray switch main
git -C gvirtual-xray pull
```

`-DXRSIM_ALLOW_UNPINNED_SUBMODULES=ON` turns the stop into a warning, but then what is built is not the version that the repository says it builds (an Engine that is ahead of the pinned release also needs `nlohmann_json`, which `vcpkg.json` does not list), so use it only to try another version on purpose.

### `Restored 0 package(s)`: the first configure builds every package

The first configuration of each preset builds the dependencies with vcpkg (10 to 20 minutes) and stores them in its binary cache (`%LOCALAPPDATA%\vcpkg\archives`); later configurations, also in new build folders and with other presets of the same triplet, restore them in seconds (`Restored 39 package(s)`). If a later configuration says `Restored 0 package(s)` and builds everything again, the hash (ABI) of the packages changed. With MinGW the cause was the `PATH`, which the triplet of vcpkg puts in the hash (another terminal, VS Code, or a runner has another `PATH`); the MinGW triplet of this repository leaves it out, and the first configuration after updating builds the packages once more. With MSVC and in the CI the other cause was the compiler hash of the tools that vcpkg builds for itself, which the CI triplets leave out (see Continuous Integration). A compiler upgrade also builds the packages again on a developer's machine, on purpose. To find another cause, configure twice with `-DVCPKG_INSTALL_OPTIONS=--debug`, copy the lines between `<abientries for zlib:` and `</abientries>` of each run, and compare them: the entry that differs is what changed.

### vcpkg waits for a filesystem lock, or the installed folder is inconsistent

Two vcpkg runs on the same vcpkg folder (two terminals, or VS Code and a terminal) wait for each other's lock (`buildtrees\vcpkg-running.lock` in `VCPKG_ROOT`): wait for the first one, or stop it. A run that was interrupted can leave the installed folder of the build inconsistent (packages missing or half installed): delete `out\build\<preset>\vcpkg_installed` and configure again; the packages come back from the binary cache.

### A build folder holds the tools of another compiler

A build folder that was configured with one compiler and is configured again with another keeps the tools in its `CMakeCache.txt`: for example MinGW's `ld.exe` and `ar.exe` under MSVC, which fails with `unrecognised emulation mode`. Delete `CMakeCache.txt` and the folder `CMakeFiles` of that build folder (or the whole folder) and configure again, or add `--fresh` to the configuration command.

### `cl` is not found

The Ninja presets with MSVC (`ninja-msvc-debug`, `ninja-msvc-release`) need a Visual Studio developer shell: `cl` is not in the `PATH` of a plain PowerShell. The Visual Studio presets (`vs2022-msvc-debug`, `vs2026-msvc-debug`) do not need it. From any PowerShell, enter an x64 developer shell, restore `VCPKG_ROOT` (see the next entry), and check the compiler before configuring:

```powershell
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
Import-Module "$vsPath\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64"
$env:VCPKG_ROOT = [Environment]::GetEnvironmentVariable("VCPKG_ROOT", "User")
Get-Command cl
```

### `VCPKG_ROOT` points to Visual Studio's own vcpkg

The Visual Studio developer shell sets `VCPKG_ROOT` to the vcpkg that comes with Visual Studio, which failed to build the packages of this repository. Use your own clone of vcpkg (one that contains the `builtin-baseline` commit of `vcpkg.json`): set `VCPKG_ROOT` as a user environment variable, and in a developer shell restore it after entering the shell (`$env:VCPKG_ROOT = [Environment]::GetEnvironmentVariable("VCPKG_ROOT", "User")`). Check it with `$env:VCPKG_ROOT` before configuring.

### The configuration of gVirtualXRay stops at `BinaryLibFiles` with `The system cannot find the path specified`

On Windows, gVirtualXRay downloads a `gzip` when none is found, and a failed download leaves an empty file, so the build stops far from the cause. Put a `gzip.exe` on the `PATH` (Git for Windows has one, with `msys-2.0.dll`, in `C:\Program Files\Git\usr\bin`; copy both to a folder of their own and add that folder, so that the other tools of Git do not hide yours) and configure again with `--fresh`.

### The tests crash right after `Creating Core instance...`

The smoke tests (`XRSim_Smoke_Test`, `XRSim_Xray_View_Test`, `XRSim_Xray_Self_Test`) segfault at start-up when `engine/` was switched to another commit after the build, because the program reads the Engine's shaders and resources from that folder at run time. Put the Engine at the recorded commit again (see the first entry) and run the tests again; a rebuild is not needed.

### The tools that the summary shows locally can differ from the ones of the runner

The summary of the CI, and the debug output of vcpkg (`-DVCPKG_INSTALL_OPTIONS=--debug`), list `cmake|<version>` and `powershell|<version>` among the entries of the hash of every package. vcpkg uses the CMake and the PowerShell of the `PATH` only when they are at least the versions that its own tool list pins, and downloads the pinned ones otherwise, so the versions in a log of this PC can differ from the versions in the summary of the runner, and so can the hashes. Compare logs of the same machine: if the versions differ between two runs on the same machine, the `PATH` changed (another CMake or PowerShell comes first), and the packages are built again.

### The tests open a window of the desktop in WSL

In WSL, WSLg sets `WAYLAND_DISPLAY` and has a Wayland socket in `XDG_RUNTIME_DIR`, so GLFW opens a real window on your desktop instead of using the virtual display of `xvfb-run`, and a key that you press in another window reaches the smoke test (`[ERROR::INPUTMANAGER::key_callback] Unknown key input: S_PRESSED` makes `XRSim_Smoke_Test` fail). Unsetting `WAYLAND_DISPLAY` alone is not enough, because the Wayland library then looks for the socket `wayland-0` in `XDG_RUNTIME_DIR`; unset both for the run: `env -u WAYLAND_DISPLAY -u XDG_RUNTIME_DIR xvfb-run -a ctest --test-dir out/build/<preset> --output-on-failure`.
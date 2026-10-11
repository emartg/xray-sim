# Sources and Attribution

This file credits the third-party libraries and resources that are part of the simulator itself and are
meant to stay in it: who made each one, where it comes from, and under which license. The engine has the
same kind of file (`docs/sources.md`), and the fork has one in its own documentation folder, with its own
naming (`doc/SOURCES.md`).

## What Is Credited Where

| Repository | File | What it credits |
|---|---|---|
| The engine (`engine/`) | [engine/docs/sources.md](../engine/docs/sources.md) | GLFW, GLM, Assimp, nlohmann/json, GoogleTest, Dear ImGui, ImGuiFileDialog, stb_image, glad, the Roboto Mono fonts, and the engine's placeholder assets |
| The gVirtualXRay fork (`gvirtual-xray/`) | [gvirtual-xray/doc/SOURCES.md](../gvirtual-xray/doc/SOURCES.md) | The gVirtualXRay project and its developers, xraylib, XrayPhysics, glad, GLEW, Assimp, zlib, and the other libraries of its build, the Welsh dragon mesh, and its other resources |
| The simulator (this repository) | This file | Whatever the simulator adds itself (below) |

The simulator is built on those two projects, which are credited in full in their own files: the engine
is the owner's OpenGL graphics engine (MIT License), and gVirtualXRay is the library of Dr Franck P.
Vidal and contributors (BSD 3-Clause License). When the simulator copies or uses an asset of either, list
it here too.

## How to Use This File

- Add an entry when a library or a resource becomes part of the simulator: its name and version, author,
  source link, and license.
- A temporary asset goes under "Placeholders". When it is kept, move it up and complete its entry; when it
  is replaced, remove it.

## Libraries

(None of its own yet: the simulator links the engine and the library above.)

## Resources

(None yet: the demo assets of the simulator will live in `resources/`.)

## Placeholders

(None yet.)

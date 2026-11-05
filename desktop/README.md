# Apex Lab Desktop

This directory contains the initial native desktop foundation for Apex Lab. It
is intentionally limited to proving that SDL3, Dear ImGui, and the
SDL_Renderer3 backend configure, build, and run together on Windows. The
historical analytics dashboard at the repository root remains a separate,
preserved application.

## Requirements

- Windows 11
- CMake 3.20 or newer
- Visual Studio 2022 with the **Desktop development with C++** workload
- Git and network access during the first CMake configure

## Dependencies

CMake downloads the dependencies at configure time with `FetchContent`:

- SDL `release-3.2.8`, built as a static library
- Dear ImGui `v1.91.9b`
- Dear ImGui SDL3 and SDL_Renderer3 backends

Only the SDL subsystems needed by this foundation are enabled; unrelated
audio, camera, controller, GPU API, and dialog subsystems are disabled.

No package manager or preinstalled SDL/ImGui SDK is required. The dependencies
are pinned to the versions already verified by the development workspace.

## Build on Windows

Run these commands from the repository root in PowerShell:

```powershell
cmake -S desktop -B desktop/build
cmake --build desktop/build --config Release
```

The generated `desktop/build/` directory is ignored by Git.

## Run

For a Visual Studio multi-configuration build:

```powershell
.\desktop\build\Release\apex_lab_desktop.exe
```

The application opens a resizable window titled **Apex Lab**. The temporary
interface confirms SDL3 and Dear ImGui initialization and provides a Close
button.

## Current scope

This phase contains only:

- SDL3 video initialization
- A resizable SDL window and SDL_Renderer3 renderer
- Dear ImGui context and backend initialization
- SDL event processing and a basic application loop
- A temporary confirmation panel with a Close button
- Ordered ImGui, renderer, window, and SDL cleanup

It does **not** yet include the existing theme, custom widgets, navigation,
screens, data models, simulation logic, assets, or JSON support.

## Provenance

The working initialization, frame-loop, event-handling, and cleanup flow was
adapted from the existing team development workspace:

- Repository: `C:\Users\cesar\Documents\ApexLabsDevSpace`
- Source commit: `c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0`
- Source file: `src/main.cpp`

Only the minimum desktop shell was ported. Existing source authorship is
preserved through this reference; this phase does not claim the full frontend
has been migrated.

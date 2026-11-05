# Apex Lab Desktop

This directory contains the native desktop foundation for Apex Lab. SDL3,
Dear ImGui, and the SDL_Renderer3 backend provide the application shell, while
the bundled Apex Lab theme and branding establish the visual foundation for
the migrated desktop screens. The historical analytics dashboard at the
repository root remains a separate, preserved application.

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
- `stb_image.h` for loading the bundled PNG artwork

Only the SDL subsystems needed by this foundation are enabled; unrelated
audio, camera, controller, GPU API, and dialog subsystems are disabled.

No package manager or preinstalled SDL/ImGui SDK is required. The dependencies
are pinned to the versions already verified by the development workspace.

The runtime assets are stored under `assets/` and copied beside the executable
after each build. Space Grotesk and IBM Plex Sans are distributed under the
SIL Open Font License; their license files are included with the font files.
The license notice for stb_image is embedded in `third_party/stb/stb_image.h`.

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

The application opens a resizable window titled **Apex Lab**.

The Start Screen provides navigation to the current desktop destinations. The
destination screens deliberately show a migration placeholder and a **Back to
Home** action until their corresponding interfaces are ported.

## Current scope

This phase contains only:

- SDL3 video initialization
- A resizable SDL window and SDL_Renderer3 renderer
- Dear ImGui context and backend initialization
- SDL event processing and a basic application loop
- The Apex Lab color theme, custom fonts, and branded image loading
- The migrated Start Screen and its responsive reference composition
- Basic navigation to Car Setup, Import / Export, Comparison, and Settings
- Explicit placeholders for screens that have not yet been migrated
- Ordered ImGui, renderer, window, and SDL cleanup

It does **not** yet include the broader vehicle data models, simulation logic,
or JSON support from the team prototype.

## Provenance

The working initialization, frame-loop, event-handling, and cleanup flow was
adapted from the existing team development workspace:

- Repository: `C:\Users\cesar\Documents\ApexLabsDevSpace`
- Source commit: `c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0`
- Source files: `src/main.cpp`, `src/app.cpp`, `src/screens/start_screen.cpp`,
  `src/widgets.cpp`, `src/theme.cpp`, and `src/textures.cpp`

Only the Start Screen and the minimum support code it requires were ported.
Existing source authorship is preserved through this reference; this phase
does not claim the full frontend has been migrated.

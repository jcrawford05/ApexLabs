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

The Start Screen provides navigation to the current desktop destinations.
Car & Track Setup is migrated for its four-column configuration UI: all five
car panels, all four track panels, Run Simulation, Previous/Next stepping,
and in-memory Reset/Save Setup. Results is migrated against sample Run #42:
header and KPIs, interactive track map with coloring modes, section times
table, telemetry chart, and section detail popup. Import / Export is
migrated for its four-card layout (import drop zone, export rows, run
history, activity log) against live setup/run state and sample history;
JSON file I/O is still stubbed. Comparison is migrated against sample run
history (run pickers, gap KPIs, setup diffs, section gaps, speed/gap
chart). Settings still shows a migration placeholder.

## Current scope

This phase contains only:

- SDL3 video initialization
- A resizable SDL window and SDL_Renderer3 renderer
- Dear ImGui context and backend initialization
- SDL event processing and a basic application loop
- The Apex Lab color theme, custom fonts, and branded image loading
- The migrated Start Screen and its responsive reference composition
- A Car & Track Setup shell with top nav, four-column layout, and category cards
- All car and track setup panels with lean in-memory setup state
- Demo Circuit map geometry and sample track / weather defaults
- Results fed by embedded sample Run #42 (map, section table, telemetry chart,
  section popup; no physics yet)
- Import / Export UI with sample run history and session activity logging
- Native Import file picker via SDL3 dialogs (Windows / macOS), plus drag-and-drop
  onto the window; JSON parse/apply is not connected yet
- Comparison Tool against sample run history
- The Results / app color palette and app-palette Button / Chip / Toggle / Dropdown
  (shared Space / Plex fonts)
- Basic navigation to Car Setup, Results, Import / Export, Comparison, and Settings
- Explicit placeholders for screens that have not yet been migrated
- Ordered ImGui, renderer, window, and SDL cleanup

It does **not** yet include JSON load/save, physics, or the broader data set
from the team prototype.

## Provenance

The working initialization, frame-loop, event-handling, and cleanup flow was
adapted from the existing team development workspace:

- Repository: `C:\Users\cesar\Documents\ApexLabsDevSpace`
- Source commit: `c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0`
- Source files: `src/main.cpp`, `src/app.cpp`, `src/screens/start_screen.cpp`,
  `src/screens/setup_screen.cpp`, `src/screens/results_screen.cpp`,
  `src/screens/import_export_screen.cpp`, `src/screens/compare_screen.cpp`,
  `src/widgets.cpp`, `src/theme.cpp`, `src/textures.cpp`, `src/track_map.*`,
  and the setup / run-result subset of `src/data.*`

Only the Start Screen, Car & Track Setup, Results, Import / Export,
Comparison, and the minimum support code they require were ported. Existing
source authorship is preserved through this reference; this phase does not
claim the full frontend has been migrated.

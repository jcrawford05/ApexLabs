# Apex Lab Desktop

This directory contains the native desktop foundation for Apex Lab. SDL3,
Dear ImGui, and the SDL_GPU backend provide the application shell, while
the bundled Apex Lab theme and branding establish the visual foundation for
the migrated desktop screens. The historical analytics dashboard at the
repository root remains a separate, preserved application.

## Requirements

- Windows 11 or macOS
- CMake 3.20 or newer
- A C++17 toolchain (Visual Studio 2022 on Windows, or Clang/AppleClang on macOS)
- Git and network access during the first CMake configure

## Dependencies

CMake downloads the dependencies at configure time with `FetchContent`:

- SDL `release-3.2.8`, built as a static library with the GPU API enabled
- Dear ImGui `v1.91.9b`
- Dear ImGui SDL3 and SDL_GPU backends
- `stb_image.h` for loading the bundled PNG artwork

Only the SDL subsystems needed by this foundation are enabled; unrelated
audio, camera, controller, and 2D-renderer subsystems are disabled.

No package manager or preinstalled SDL/ImGui SDK is required. The dependencies
are pinned to the versions already verified by the development workspace.

The runtime assets are stored under `assets/` and copied beside the executable
after each build. Space Grotesk and IBM Plex Sans are distributed under the
SIL Open Font License; their license files are included with the font files.
The license notice for stb_image is embedded in `third_party/stb/stb_image.h`.

## 3D vehicle preview asset

The Car Setup screen includes an SDL_GPU viewport that loads:

- **Extension:** `.glb` (binary glTF)
- **Path:** `desktop/assets/vehicles/formula_car/apex_viewport_test.glb`

`.glb` is preferred over `.gltf` because it keeps geometry and materials in
one file. The current loader flattens triangle meshes and material base
colors into the preview. A modular Formula-style car with separately named
nodes is ideal for later wing/tire focusing.

## Build on Windows

Run these commands from the repository root in PowerShell:

```powershell
cmake -S desktop -B desktop/build
cmake --build desktop/build --config Release
```

The generated `desktop/build/` directory is ignored by Git.

If you previously configured with `SDL_GPU` disabled, delete `desktop/build`
and re-run configure so the GPU backend is enabled.

## Build on macOS

```bash
./desktop/rebuild.sh
# or: cmake -S desktop -B desktop/build && cmake --build desktop/build --config Release
```

## Run

For a Visual Studio multi-configuration build:

```powershell
.\desktop\build\Release\apex_lab_desktop.exe
```

On macOS / single-config generators:

```bash
./desktop/build/apex_lab_desktop
```

The application opens a resizable window titled **Apex Lab**.

The Start Screen provides navigation to the current desktop destinations.
Car Setup and Track Setup are separate screens: all five car panels and all
four track panels, Run Simulation, Move to Track Configuration, Previous/Next
stepping, and in-memory Reset/Save Setup. Car Setup also shows a 3D preview
viewport (drag to orbit, scroll to zoom). Results is migrated against sample Run #42:
header and KPIs, interactive track map with coloring modes, section times
table, telemetry chart, and section detail popup. Import / Export is
migrated for its four-card layout (import drop zone, export rows, run
history, activity log) against live setup/run state and sample history;
JSON file I/O is still stubbed. Comparison is migrated against sample run
history (run pickers, gap KPIs, setup diffs, section gaps, speed/gap
chart). Settings is fully migrated (Simulation, Units & display, Data &
storage, About) with live prefs, Save / Reset, and CHANGED rows. Speed and
temperature units update Results, Comparison, and Setup displays immediately;
Data path / clear-history remain stubbed until persistence lands.

## Current scope

This phase contains:

- SDL3 video + SDL_GPU device / swapchain initialization
- Dear ImGui context with the SDL3 + SDL_GPU backends
- SDL event processing and a basic application loop
- The Apex Lab color theme, custom fonts, and branded GPU image loading
- The migrated Start Screen and its responsive reference composition
- Separate Car Setup and Track Setup screens with top nav and category cards
- A Car Setup 3D viewport that loads `apex_viewport_test.glb` (orbit/zoom)
- All car and track setup panels with lean in-memory setup state
- Demo Circuit map geometry and sample track / weather defaults
- Results fed by embedded sample Run #42 (map, section table, telemetry chart,
  section popup; no physics yet)
- Import / Export UI with sample run history and session activity logging
- Native Import file picker via SDL3 dialogs (Windows / macOS), plus drag-and-drop
  onto the window; JSON parse/apply is not connected yet
- Comparison Tool against sample run history
- Settings (Simulation, Units, Data & storage, About) with sticky section nav,
  Save / Reset, unsaved-change chrome, and unit-aware Results / Compare / Setup
- The Results / app color palette and app-palette Button / Chip / Segmented /
  Toggle / Dropdown / NumberInput / TickSlider / Badge (shared Space / Plex fonts)
- Basic navigation to Car Setup, Track Setup, Results, Import / Export, Comparison, and Settings
- Ordered ImGui, GPU device, window, and SDL cleanup

It does **not** yet include animated part focusing, JSON load/save, physics, or
the broader data set from the team prototype.

## Provenance

The working initialization, frame-loop, event-handling, and cleanup flow was
adapted from the existing team development workspace:

- Repository: `C:\Users\cesar\Documents\ApexLabsDevSpace`
- Source commit: `c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0`
- Source files: `src/main.cpp`, `src/app.cpp`, `src/screens/start_screen.cpp`,
  `src/screens/car_setup_screen.cpp`, `src/screens/track_setup_screen.cpp`,
  `src/setup_common.*`, `src/screens/results_screen.cpp`,
  `src/screens/import_export_screen.cpp`, `src/screens/compare_screen.cpp`,
  `src/screens/settings_screen.cpp`, `src/widgets.cpp`, `src/theme.cpp`,
  `src/textures.cpp`, `src/track_map.*`, and the setup / run-result subset of
  `src/data.*`

Only the Start Screen, Car Setup, Track Setup, Results, Import / Export,
Comparison, Settings, and the minimum support code they require were ported.
Existing source authorship is preserved through this reference; this phase does
not claim the full frontend has been migrated.

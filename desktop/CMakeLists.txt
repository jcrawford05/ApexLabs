cmake_minimum_required(VERSION 3.20)

project(ApexLabDesktop VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# Keep the dependency versions aligned with the verified ApexLabsDevSpace
# implementation at commit c0998baaf04bb2bc3c53ec999eb4e05d6ab141f0.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
# This foundation needs only SDL's event, video, and renderer paths. Keeping
# unrelated subsystems out reduces compile time and generated build size.
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_GPU OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.8
  GIT_SHALLOW TRUE)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(SDL3 imgui)

# Dear ImGui does not provide its own CMake project. Build only the core and
# the SDL3 + SDL_Renderer3 backends required by this foundation application.
add_library(apex_imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)

target_include_directories(apex_imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends)
target_link_libraries(apex_imgui PUBLIC SDL3::SDL3)

add_executable(apex_lab_desktop
  src/app.cpp
  src/main.cpp
  src/run_result.cpp
  src/screens/results_screen.cpp
  src/screens/setup_screen.cpp
  src/screens/start_screen.cpp
  src/theme.cpp
  src/textures.cpp
  src/track_map.cpp
  src/track_setup.cpp
  src/vehicle.cpp
  src/widgets.cpp)
target_include_directories(apex_lab_desktop PRIVATE src third_party)
target_link_libraries(apex_lab_desktop PRIVATE apex_imgui)

if(MSVC)
  target_compile_options(apex_lab_desktop PRIVATE /W4 /utf-8)
else()
  target_compile_options(apex_lab_desktop PRIVATE -Wall -Wextra)
endif()

if(WIN32)
  set_target_properties(apex_lab_desktop PROPERTIES
    WIN32_EXECUTABLE $<CONFIG:Release>)
endif()

add_custom_command(TARGET apex_lab_desktop POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:apex_lab_desktop>/assets
  COMMENT "Copying Apex Lab desktop assets")

cmake_minimum_required(VERSION 3.20)
project(ApexLabsDB VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# --- SQLite ------------------------------------------------------------------
# macOS and most Linux installs already have SQLite. If not (usually Windows),
# download the official single-file "amalgamation" and build it here.
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
  message(STATUS "Using system SQLite ${SQLite3_VERSION}")
else()
  message(STATUS "System SQLite not found; downloading the amalgamation")
  FetchContent_Declare(sqlite_amalgamation
    URL https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip)
  FetchContent_MakeAvailable(sqlite_amalgamation)
  add_library(sqlite3_bundled STATIC ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c)
  target_include_directories(sqlite3_bundled PUBLIC ${sqlite_amalgamation_SOURCE_DIR})
  add_library(SQLite::SQLite3 ALIAS sqlite3_bundled)
endif()

# --- nlohmann_json (same version as the ImGui app) ---------------------------
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "")
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d)
FetchContent_MakeAvailable(nlohmann_json)

# --- The storage library: this is the part the ImGui app will link -----------
add_library(apex_db STATIC src/apex_db.cpp)
target_include_directories(apex_db PUBLIC src)
target_link_libraries(apex_db PUBLIC SQLite::SQLite3 nlohmann_json::nlohmann_json)

# --- Demo CLI ----------------------------------------------------------------
add_executable(apexdb src/demo_main.cpp)
target_link_libraries(apexdb PRIVATE apex_db)
target_compile_definitions(apexdb PRIVATE APEX_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

foreach(t apex_db apexdb)
  if(MSVC)
    target_compile_options(${t} PRIVATE /W4 /utf-8)
  else()
    target_compile_options(${t} PRIVATE -Wall -Wextra)
  endif()
endforeach()

# schema.sql and the sample data go next to the executable.
add_custom_command(TARGET apexdb POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/schema.sql $<TARGET_FILE_DIR:apexdb>/schema.sql
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:apexdb>/data)

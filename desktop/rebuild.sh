#!/usr/bin/env bash
# Configure (if needed), build, and optionally run Apex Lab Desktop.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build"
CONFIG="${CONFIG:-Release}"
RUN=0

usage() {
  cat <<'EOF'
Usage: ./desktop/rebuild.sh [--run] [--config Debug|Release]

  --run              Launch apex_lab_desktop after a successful build
  --config <name>    Build configuration (default: Release)
  -h, --help         Show this help

Can be run from the repository root or from desktop/.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --run)
      RUN=1
      shift
      ;;
    --config)
      CONFIG="${2:?--config requires Debug or Release}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

NEED_CONFIGURE=0
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  NEED_CONFIGURE=1
elif ! grep -q "^SDL_GPU:BOOL=ON$" "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null; then
  # Older configures disabled the GPU API; force a fresh configure.
  NEED_CONFIGURE=1
fi

if [[ "${NEED_CONFIGURE}" -eq 1 ]]; then
  echo "Configuring desktop → ${BUILD_DIR}"
  cmake -S "${ROOT}" -B "${BUILD_DIR}"
fi

echo "Building (${CONFIG})"
cmake --build "${BUILD_DIR}" --config "${CONFIG}"

# Multi-config generators (e.g. Visual Studio) put the binary under build/<Config>/.
# Single-config generators (e.g. Ninja/Unix Makefiles on macOS) put it in build/.
CANDIDATES=(
  "${BUILD_DIR}/${CONFIG}/apex_lab_desktop"
  "${BUILD_DIR}/${CONFIG}/apex_lab_desktop.exe"
  "${BUILD_DIR}/apex_lab_desktop"
  "${BUILD_DIR}/apex_lab_desktop.exe"
)

APP=""
for candidate in "${CANDIDATES[@]}"; do
  if [[ -x "${candidate}" ]]; then
    APP="${candidate}"
    break
  fi
done

if [[ -z "${APP}" ]]; then
  echo "Build finished, but apex_lab_desktop was not found under ${BUILD_DIR}." >&2
  exit 1
fi

echo "Built ${APP}"

if [[ "${RUN}" -eq 1 ]]; then
  echo "Launching…"
  exec "${APP}"
fi

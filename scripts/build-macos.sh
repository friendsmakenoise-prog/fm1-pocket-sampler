#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"

if ! xcrun --find clang++ >/dev/null 2>&1; then
  echo "Apple command-line developer tools are missing."
  echo "Run: xcode-select --install"
  exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is missing."
  echo "If you use Homebrew, run: brew install cmake"
  echo "Then run this script again."
  exit 1
fi

echo "Configuring FM-1 Pocket Sampler..."
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build "$BUILD" --config Release

echo "Running tests..."
ctest --test-dir "$BUILD" --output-on-failure

APP="$BUILD/fm1_desktop.app"
BIN="$BUILD/fm1_desktop"

if [[ -d "$APP" ]]; then
  echo "Opening $APP"
  open "$APP"
elif [[ -x "$BIN" ]]; then
  echo "Opening $BIN"
  "$BIN" &
else
  echo "Build succeeded, but the desktop executable was not found where expected."
  exit 1
fi

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"

if ! command -v xcrun >/dev/null 2>&1; then
  echo "Apple developer tools are not available."
  echo "Run: xcode-select --install"
  exit 1
fi

if ! xcrun --find clang++ >/dev/null 2>&1; then
  echo "Apple C++ compiler is missing."
  echo "Run: xcode-select --install"
  exit 1
fi

if ! xcrun --find clang >/dev/null 2>&1; then
  echo "Apple C compiler is missing."
  echo "Run: xcode-select --install"
  exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is missing."
  echo "If you use Homebrew, run: brew install cmake"
  echo "Then run this script again."
  exit 1
fi

SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
CXX="$(xcrun --find clang++)"
CC="$(xcrun --find clang)"
export SDKROOT

echo "FM-1 Pocket Sampler macOS build"
echo "SDK:      $SDKROOT"
echo "C++:      $CXX"
echo "C:        $CC"
echo "CMake:    $(command -v cmake)"
echo

# Verify that Apple's libc++ headers can actually be resolved before asking
# CMake to build the project. Some Command Line Tools installations need the
# SDK sysroot supplied explicitly even though xcrun can locate the SDK.
CHECK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/fm1-build-check.XXXXXX")"
trap 'rm -rf "$CHECK_DIR"' EXIT
printf '#include <cstdint>\nint main() { return 0; }\n' > "$CHECK_DIR/check.cpp"

if ! "$CXX" -std=c++17 -stdlib=libc++ -isysroot "$SDKROOT" \
    "$CHECK_DIR/check.cpp" -o "$CHECK_DIR/check" >/dev/null 2>&1; then
  echo "The Apple compiler could not resolve the C++ standard library headers."
  echo "SDKROOT was: $SDKROOT"
  echo
  echo "Try:"
  echo "  sudo xcode-select --reset"
  echo "  sudo xcode-select --switch /Library/Developer/CommandLineTools"
  echo
  echo "Then run this script again."
  exit 1
fi

echo "C++ SDK check passed."
echo "Configuring FM-1 Pocket Sampler..."

cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_SYSROOT="$SDKROOT" \
  -DCMAKE_C_COMPILER="$CC" \
  -DCMAKE_CXX_COMPILER="$CXX"

echo "Building..."
cmake --build "$BUILD" --config Release

echo "Running tests..."
ctest --test-dir "$BUILD" --output-on-failure

APP="$BUILD/fm1_desktop.app"
BIN="$BUILD/fm1_desktop"

if [[ -d "$APP" ]]; then
  echo
  echo "Build succeeded. Opening $APP"
  open "$APP"
elif [[ -x "$BIN" ]]; then
  echo
  echo "Build succeeded. Opening $BIN"
  "$BIN" &
else
  echo "Build succeeded, but the desktop executable was not found where expected."
  exit 1
fi

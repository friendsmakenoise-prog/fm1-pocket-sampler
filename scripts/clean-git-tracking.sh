#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "This helper must be run inside the Git checkout."
  exit 1
fi

echo "Removing generated files from Git tracking..."
git rm -r --cached --ignore-unmatch build >/dev/null 2>&1 || true
git rm --cached --ignore-unmatch .DS_Store >/dev/null 2>&1 || true

# Start tomorrow with a genuinely clean local build tree too.
rm -rf build
find . -name .DS_Store -type f -delete 2>/dev/null || true

echo "Done. build/ and .DS_Store are now ignored."
echo "GitHub Desktop should show the tracked build artefacts as deletions; commit and push them once."
echo "The next ./scripts/build-macos.sh will recreate build/ locally without adding it to Git."

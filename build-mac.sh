#!/bin/bash
# Build and optionally run the simulator on macOS.
set -e
cd "$(dirname "$0")"

if ! xcode-select -p &>/dev/null; then
  echo "Xcode Command Line Tools are required."
  echo "Install with:  xcode-select --install"
  exit 1
fi

make clean 2>/dev/null || true
make

echo ""
echo "Build OK: bin/ComputerGraphicsLab"
echo "Run with:  ./bin/ComputerGraphicsLab"
echo "Or:        make run"
echo ""

if [[ "${1:-}" == "--run" ]]; then
  exec ./bin/ComputerGraphicsLab
fi

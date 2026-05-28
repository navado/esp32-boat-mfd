#!/usr/bin/env bash
set -euo pipefail

CONTAINER="${SIGNALK_CONTAINER:-signalk-server}"

pkill -f "tools/fake_boat.py" 2>/dev/null || true
docker stop "$CONTAINER" >/dev/null 2>&1 || true

echo "demo stopped."

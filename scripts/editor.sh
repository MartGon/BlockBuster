#!/usr/bin/env bash
# Launches the map editor from the repository root, where it finds ./shaders, ./maps and editor.ini.
# Without options, the editor reopens the last map it had open.
#
# Usage: scripts/editor.sh [-m map | -n] [-d build_dir]
#   -m  Open this map, a folder inside the maps folder
#   -n  Start with a new, empty map
#   -d  Build directory (default: ./build)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MAP=""
NEW_MAP=0
BUILD_DIR="$ROOT/build"

usage()
{
    sed -n '2,9p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

while getopts "m:nd:h" opt; do
    case "$opt" in
        m) MAP="$OPTARG" ;;
        n) NEW_MAP=1 ;;
        d) BUILD_DIR="$(realpath "$OPTARG")" ;;
        h) usage 0 ;;
        *) usage 1 ;;
    esac
done

if [[ -n "$MAP" && "$NEW_MAP" == 1 ]]; then
    echo "-m and -n can't be used together" >&2
    exit 1
fi

EDITOR_BIN="$BUILD_DIR/src/apps/editor/Editor"
if [[ ! -x "$EDITOR_BIN" ]]; then
    echo "Missing $EDITOR_BIN. Build first: (cd \"$BUILD_DIR\" && ninja Editor)" >&2
    exit 1
fi

# The editor reads its settings and resolves relative paths from the working directory
cd "$ROOT"
CONFIG="editor.ini"

# The editor has no command line options. It opens the map named in editor.ini, and saves the open one there on exit
if [[ -n "$MAP" || "$NEW_MAP" == 1 ]]; then
    if [[ ! -f "$CONFIG" ]]; then
        echo "No $CONFIG yet. Run the editor once without options to create it" >&2
        exit 1
    fi

    if [[ -n "$MAP" ]]; then
        MAPS_FOLDER="$(sed -n 's/^MapsFolder=//p' "$CONFIG")"
        MAPS_FOLDER="${MAPS_FOLDER:-./maps}"
        MAPS_FOLDER="${MAPS_FOLDER%/}"
        if [[ ! -f "$MAPS_FOLDER/$MAP/$MAP.bbm" ]]; then
            echo "Map not found: $MAPS_FOLDER/$MAP/$MAP.bbm" >&2
            echo "Available maps: $(ls "$MAPS_FOLDER" 2>/dev/null | tr '\n' ' ')" >&2
            exit 1
        fi
    fi

    if grep -q '^Map=' "$CONFIG"; then
        sed -i "s|^Map=.*|Map=$MAP|" "$CONFIG"
    else
        echo "Map=$MAP" >> "$CONFIG"
    fi
fi

exec "$EDITOR_BIN"

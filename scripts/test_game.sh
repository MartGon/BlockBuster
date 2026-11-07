#!/usr/bin/env bash
# Launches a local test game: a server with bots plus one client connected to it directly,
# without the match-making server. The server is stopped when the client exits.
#
# Usage: scripts/test_game.sh [-m map] [-b bots] [-D difficulty] [-g gamemode] [-p port] [-d build_dir]
#   -m  Map name, a folder inside ./maps (default: Alpha3)
#   -b  Number of bots (default: 1)
#   -D  Bot difficulty: easy, normal, hard (default: normal)
#   -g  Game mode: "Free for All", "Team DeathMatch", "Domination", "Capture the Flag", "Gun Game" (default: Free for All)
#   -p  Server port (default: 8080)
#   -d  Build directory (default: ./build)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MAP="Alpha3"
BOTS=1
BOT_DIFFICULTY="normal"
MODE="Free for All"
PORT=8080
BUILD_DIR="$ROOT/build"

usage()
{
    sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

while getopts "m:b:D:g:p:d:h" opt; do
    case "$opt" in
        m) MAP="$OPTARG" ;;
        b) BOTS="$OPTARG" ;;
        D) BOT_DIFFICULTY="$OPTARG" ;;
        g) MODE="$OPTARG" ;;
        p) PORT="$OPTARG" ;;
        d) BUILD_DIR="$(realpath "$OPTARG")" ;;
        h) usage 0 ;;
        *) usage 1 ;;
    esac
done
shift $((OPTIND - 1))

# Catches typos like "-gm X", which getopts reads as "-g m" plus a stray "X"
if [[ $# -gt 0 ]]; then
    echo "Unexpected arguments: $*" >&2
    usage 1
fi

SERVER_BIN="$BUILD_DIR/src/server/Server"
CLIENT_BIN="$BUILD_DIR/src/apps/client/Client"
MAP_FILE="$ROOT/maps/$MAP/$MAP.bbm"

for bin in "$SERVER_BIN" "$CLIENT_BIN"; do
    if [[ ! -x "$bin" ]]; then
        echo "Missing $bin. Build first: (cd \"$BUILD_DIR\" && ninja Server Client)" >&2
        exit 1
    fi
done

if [[ ! -f "$MAP_FILE" ]]; then
    echo "Map not found: $MAP_FILE" >&2
    echo "Available maps: $(ls "$ROOT/maps" | tr '\n' ' ')" >&2
    exit 1
fi

if ss -Huln "sport = :$PORT" | grep -q .; then
    echo "Port $PORT is already in use. Is another server running? Use -p to pick another port." >&2
    exit 1
fi

# Both binaries use paths relative to the working directory (./maps, ./logs, ./client.ini)
cd "$ROOT"
mkdir -p logs

# The match starts once every bot and the human player have joined
STARTING_PLAYERS=$((BOTS + 1))

echo "Starting server: map=$MAP mode=\"$MODE\" bots=$BOTS difficulty=$BOT_DIFFICULTY port=$PORT"
"$SERVER_BIN" -m "$MAP_FILE" -p "$PORT" -sp "$STARTING_PLAYERS" -b "$BOTS" -bd "$BOT_DIFFICULTY" -gm "$MODE" -v 1 > logs/server.out 2>&1 &
SERVER_PID=$!

CLIENT_PID=""
cleanup()
{
    if [[ -n "$CLIENT_PID" ]] && kill -0 "$CLIENT_PID" 2>/dev/null; then
        kill "$CLIENT_PID"
        wait "$CLIENT_PID" 2>/dev/null || true
    fi
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "Stopping server"
        kill "$SERVER_PID"
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

# Wait for the server to start listening
for _ in $(seq 50); do
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "Server exited during startup. See logs/server.out and logs/server.log" >&2
        exit 1
    fi
    ss -Huln "sport = :$PORT" | grep -q . && break
    sleep 0.1
done

echo "Starting client. Logs: logs/server.log, logs/client-test.log"
# Run in the background and wait, so Ctrl+C triggers the cleanup right away
BB_DIRECT_CONNECT="127.0.0.1:$PORT:$MAP" "$CLIENT_BIN" test > logs/client.out 2>&1 &
CLIENT_PID=$!
wait "$CLIENT_PID" || true

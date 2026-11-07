# Plan: Editor play-test button

## Context
Testing a map today means saving it, leaving the editor and running `scripts/test_game.sh -m <map> ...` by hand. The button does that from the editor, with the same script, so both paths behave the same.

## Design
- **Entry points:** a new "Play" menu with "Play test..." (F5) and "Stop play test". F5 opens a popup with:
  - Game mode: only the modes the map supports (from `ValidateMap`). If none is supported, the reason is shown and Start is disabled.
  - Bots (0–15, default 3) and bot difficulty (easy / normal / hard).
  - Validation errors, if any, as a warning. They don't block the launch.
- **Saving:** a new map must be saved first (Save As). An already-saved map with unsaved changes is saved automatically on Start.
- **Launch:** `PlayTest` (`src/apps/editor/PlayTest.{h,cpp}`) spawns `bash scripts/test_game.sh -m <map> -b <bots> -D <difficulty> -g <mode>` without waiting for it. Its output goes to `logs/playtest.out`.
  - The script path is the `PlayTestScript` config option (default `./scripts/test_game.sh`).
  - The script only finds maps in `<repo>/maps`, so the editor refuses to launch when its maps folder is elsewhere.
- **Status:** the process is polled every frame (no zombies). The menu bar shows "Play test running", or "Play test failed: <last log line>" when the script exits with an error. One play test at a time.
- **Stop:** SIGTERM to the script, whose trap stops the client and server. The editor also stops a running play test when it quits.
- **Platforms:** Linux/macOS only, since it needs bash. On Windows the menu entry shows a "not supported" message.

## Verification
- Build the editor. Launch a play test on a saved map; check that the game starts, that Stop closes it, and that a failure (e.g. port in use) shows in the menu bar.

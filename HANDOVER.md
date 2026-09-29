# FrogComposband: RVIP handover

RVIP stages 1–9 done (2026-09-26). Live: https://ruzzoli.de/roguelikes/frogcomposband/,
repo https://github.com/memmaker/frogcomposband (`master`), shrine
https://ruzzoli.de/roguelikes/shrine/frogcomposband.html. Procedure:
`~/Games/rvip-tools/RVIP.md`.

## Source
- **Case A** (z-term, `lib/edit`, `lib/pref`). Remote `upstream` =
  https://github.com/sulkasormi/frogcomposband, base `master` @ `3d28f6b1`
  (2023-08-02, 7.1.salmiak.6). Other upstream branches are older.
- Some sources are CRLF (e.g. `save.c`): edit bytes,
  keep the line endings.
- Lineage (from `lib/file/credits.txt`, used in index tree): Hengband →
  Chengband 2010 (Kousky, Zhang, Levy) → PosChengband 2012 (Chris Kousky) →
  ComPosband 2017 (Gwilim Owen) → FrogComposband 2018 (Antero Sulka).

## Build, test, deploy
- `sh web/build.sh` → `web/dist`. emcc over `CFILES ZFILES ANGFILES` of
  `src/Makefile.src` (CRLF stripped) + `main.c main-web.c`, Asyncify, IDBFS;
  `DEFAULT_*_PATH` must be passed with `-D` (autotools-only macros). Preload
  `lib/{edit,file,help,pref}` + web `sound.cfg` as `/frogcomposband/lib`.
  Pages load the shared `../rvip-wm.js` / `../rvip-app.js`; fonts from
  `~/Games/roguelikes-index/fonts` (`fonts.json`).
- `web/deploy.sh` → `ruzzoli.de/roguelikes/frogcomposband/` (needs commit +
  push first).
- Native ASan test build (ad hoc, no script): `-DUSE_GCU -DUSE_NCURSES
  -fsanitize=address`, isolated `HOME` + copied `lib`, pty random keys.
  `.github/workflows/release.yml` builds Linux/macOS/Windows releases.
- Quick start: title "Press any key" → `b` (Beginner) RET RET = Hobbit Rogue.
  Beginner mode has **no up staircases** (shafts): use Normal to test `<`.
  Cheats: `!` `Y:allow_debug_opts`, then `^W` / `^A`.

## Port map (`USE_WEB`)
- `src/main-web.c`: web frontend (TinyAngband-style z-term), `init_web()`
  called directly from `main.c`; 8 terms (`WEB_TERMS`, window flags from
  `web_window_flags[]`, set before load). Text windows, Status (sidebar +
  status line) and pop-ups (term 0 while icky / after `Term_save()`) are HTML
  lines; the map is the only canvas. New `PW_PLAYER` Character window
  (`fix_player()` in `py_info.c`, page 1 of the sheet).
- `save.c` `web_sync_files()` after `save_player()`; `z-config.h` no
  `SAFE_SETUID`; `floors.c` `init_saved_floors()` forces deletion of leftover
  temp floor files (reload without quitting).
- `message.c` `msg_line_flush()` never waits at `-more-` (Frog has no
  `auto_more` option); prompts still wait.
- Explore (`X`, original keyset only; `X` removed from the `command_menu`
  trigger keys): end of `src/cmd2.c` (port of Zangband's), hooks in
  `dungeon.c` `process_command()` / `process_player()` / `dungeon()`,
  `disturb()` and `cmsg_print()` stop it. Paints every step (40 ms). `<`/`>`
  off the stairs travel to the nearest known staircase (`travel_begin()`); the
  next press takes them. On the surface only the current town square.
- Enter menu: `util.c` `inkey_from_menu()` rewritten as Zangband's
  `cmd_menu()` (`cmd_menu_list[]`, keys of the current keyset). Item menus:
  `gear_ui()` in `obj.c` + cursor in every `obj_prompt()` (`obj_prompt.c`);
  actions run via `obj_prompt_preselect` + `queue_raw_command()` and the list
  reopens (`gear_reopen`) unless `hostile_in_view()`.
- Tiles: **Shockbolt** 64x64 (`web/tiles.webp`, 13 MB, from tactical-angband;
  own 16x16 set covers only 59%). `lib/pref/graf-shb.prf` from
  `python3 web/mkgraf-shb.py`; coverage `python3 web/tile-coverage.py`
  (99.9%). `GRAPHICS_SHOCKBOLT 3`, big-tile mode; `UI_MAP_STEP` fixes Frog's
  viewport code for big tiles (`xtra2.c`, `cmd3.c`, `wild.c`). `FF_PLANT`
  gets grass underneath (`cave.c` `map_info()`).
- Game end: `close_game()` → `quit()` → `hook_quit` → `js_quit`; dead =
  reload into a new birth. Run-end beacon: `close_game()` is_dead branch →
  `web_run_end()` (win checked first via `total_winner`; "Quitting" etc. =
  quit). Killer art: roguelikes-index `killers/make.py` `frogcomposband()`.
- Sound: `web/sounds.py` writes the web `sound.cfg` + Dubtrain wavs; music
  `web/music/new_town.ogg` (depth 0). Help: `web/make-help.py` from the Docs
  entry `~/Desktop/Games/Roguelikes/Docs` (`frogcomposband.html`).
- Layout file `/frogcomposband/lib/user/web-layout.json`.

## Open problems
- Explore stops each time it walks over an already-visited item ("You see
  ..."); a gap in the known map (after teleport) gives "You know of no way
  down" although a `>` is visible; no explore key in the roguelike keyset.
- Item lists: the reopened list hides the action's message (only in `^P`);
  Tab/^E/^P/^Q/^F/^W in `i`/`e` examine instead of switching tab; cursor starts
  at the top after reopening; the item menu box can cover the list.
- Tiles: 1063 family stand-ins; bookstore uses the "Temple" tile;
  swamp/slush/toxic waste = water, snow = sand, glacier/ice = quartz; `M`
  squashes tiles; 13 MB `tiles.webp` loads slowly.
- No high-score list after death (upstream); Character window = page 1 only;
  a dead character's message history may carry into the new one; music plays
  on the whole surface.
- Beacon: real Serpent kill not tested (win tested via a temporary build).

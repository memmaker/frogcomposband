# FrogComposband: RVIP handover

## RVIP progress

### Stage 1 (get + build): done 2026-09-26
- Folder `~/Games/frogcomposband`, **case A** (z-term, `lib/edit`, `lib/pref`).
  Full upstream history, remote renamed `upstream`
  (https://github.com/sulkasormi/frogcomposband); our commits on `master`.
- Base: upstream **`master` @ `3d28f6b1`** (2023-08-02, "display time-lord
  stasis power"), **FrogComposband 7.1.salmiak.6** (`defines.h`: 7.1
  "salmiak", VER_EXTRA 6), 63 commits after the last tag `v7.1.salmiak`
  (`36ae9d54`). Other branches are older (`develop` 7.0.3 2017, `unstable`
  2016, `2.0-release` 2013); master builds, so master.
- Web frontend: `src/main-web.c` (TinyAngband's, W3 pattern: this z-term
  has `TERM_XTRA_CLEAR` and `bigcurs_hook` like TinyAngband, unlike
  Zangband 2.7). `main.c` has no `modules[]` table: plain `if (!done)`
  chain, so `init_web(int, char**)` is called directly under `USE_WEB` and
  sets `ANGBAND_SYS = "x11"` (no INIT_MODULE cast). Page: `web/index.html`,
  `web/frogcomposband.js` (from Zangband's, `Module.qb`, IDBFS on
  `lib/save|user|apex|bone`, save = `0.PLAYER`), shared `rvip-wm.js`
  copied by the build. Text only (tiles are stage 4).
- Build: `sh web/build.sh` → `web/dist`. `emcc -O2 -fcommon -std=gnu99
  -DUSE_WEB -w -DDEFAULT_{CONFIG,LIB,DATA}_PATH='"./lib/"'` over
  `CFILES ZFILES ANGFILES` of `src/Makefile.src` (CRLF stripped; that list
  is what autotools uses) + `main.c main-web.c`; `-sASYNCIFY
  -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 -sALLOW_MEMORY_GROWTH
  -sINITIAL_MEMORY=64MB -sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web`,
  preload `lib/{edit,file,help,pref}` + empty `data info save user apex
  bone script` as `/frogcomposband/lib`. **Zero wasm-ld warnings.**
  `-Wcast-function-type-strict` only shows `vec_free_f`/`vec_cmp_f`/
  `int_map_free_f` casts (typed pointer ↔ `void *`, same wasm signature:
  harmless).
- Port edits (`USE_WEB`): `z-config.h` no `SAFE_SETUID`; `save.c`
  `web_sync_files()` at the end of `save_player()`; `main.c` web entry.
  `PRIVATE_USER_PATH` is already off upstream. No fork/locking/`long`
  changes needed. Many sources are CRLF (`save.c`, `cmd4.c`): edit bytes.
- Quirks: the `DEFAULT_*_PATH` macros come from autotools only (pass them
  with `-D`). Birth starts with a game-type menu (b Beginner / n Normal /
  m Monster); Beginner → RET → RET is a quick start (Hobbit Rogue). Title
  screen is a fake game view ("Press any key"). `auto_more` is **not an
  option** here: `message.c` has a transient `auto_more_state`
  (`AUTO_MORE_PROMPT/SKIP_ONE/SKIP_BLOCK/SKIP_ALL`) → step 3d needs a code
  change (e.g. force `AUTO_MORE_SKIP_ALL`-like behaviour in the web build).
  `init_web()` sets `center_player` o_norm only.
- ASan: native curses build (`-DUSE_GCU -DUSE_NCURSES -fsanitize=address`,
  isolated `HOME` + copied `lib`, pty driver, random keys without `Q _ @`):
  8 seeds × (3000 keys from a new Beginner character, Ctrl-X save, 2000
  keys after restore). One upstream bug, fixed in `port:` commit
  `5b0104a5`: `cmd4.c` `do_cmd_macro_aux()` macro trigger key burst
  overflowed `buf[1024]` (now max 255 keys). Then clean. Objects deleted.
- Browser test (own tab, 127.0.0.1): title → Beginner quick start → town
  (Outpost) → 800 random keys (walked into the wilderness), no console
  errors, Ctrl-X "Saving game... done" → reload → restored in place. Test
  IDBFS databases (`/frogcomposband/lib/*`) deleted.
- **Tiles decision (stage 4): Shockbolt** (case A fallback). Only own sets
  are `lib/xtra/graf/8x8` and `16x16` (Adam Bolt, `graf-new.prf` +
  `xtra-new.prf`, no 32x32); 16x16 covers **1259/2119 = 59.4%**
  (`python3 web/tile-coverage.py`: monsters 806/1395, objects 401/536 by
  `I:tval:sval`, features 52/188). Below 95%.
- Open problems: sub-windows (Inventory/Visible/Messages) stay empty: no
  window flags set for the web terms (stage 5). No auto_more (see quirks).

### Next: stage 2 (explore + stairs)
- Copy Zangband's explore (`~/Games/zangband/src/cmd2.c` end:
  `explore_step()`, `do_cmd_explore()`, `explore_to_stairs()`, hooks in
  `dungeon.c` `process_command()` / `process_player()` / `dungeon()`),
  adapted to this map model.
- Map: `cave[y][x]` → `cave_type` (`feat`, `mimic`, `info` with `CAVE_MARK`
  for known, `o_idx`, `m_idx`); features by flags (`cave_have_flag_bold(y,
  x, FF_...)`, `FF_LESS 22`/`FF_MORE`, `feat_up_stair`/`feat_down_stair`
  globals). Traps are features (`FF_TRAP`).
- Main loop: `process_player()` (`dungeon.c` ~l.4533) → `process_command()`
  (~l.4454); travel already runs as a per-turn step (`travel_step()` in
  `cmd1.c` ~l.6437, called from `process_player()` ~l.4971), with
  `travel_begin(mode, x, y)` (`cmd2.c` ~l.3900, flow pathing) and
  `travel_cancel()`; `` ` `` = travel to a target, `J` = resume travel,
  **`H` is taken** (`do_cmd_get_nearest()`, walk to nearest item), `_` =
  autopick editor. Stairs walk can reuse `travel_begin()`.
- Wilderness: town is part of the wilderness (`p_ptr->wild_mode` = world
  map mode, travel is disabled there); `dun_level` global, 0 = surface.

### Stage 2 (explore + stairs): done 2026-09-26
- **Explore key `X`** (original keyset; `H` is `do_cmd_get_nearest()`,
  `` ` `` is travel). `X` was one of the keys that open Frog's own command
  menu (`util.c` `request_command()`: Enter / `x` / `X` when the
  `command_menu` option is on); `X` is dropped from that test, Enter and
  `x` still open the menu. Roguelike keyset: `X` is a keymap to `n`
  (repeat), so no explore key there.
- Code: end of `src/cmd2.c` (port of Zangband's explore): `auto_explore`,
  `explore_stairs`, `explore_new_level()`, `explore_find()` (BFS),
  `explore_step()`, `do_cmd_explore()`, `explore_to_stairs()`,
  `explore_stairs_arrive()`; prototypes in `externs.h`.
- Hooks: `process_command()` `case 'X'` (`dungeon.c`, not in `wild_mode`);
  `process_player()`: `auto_explore` in the key-abort check, `else if
  (auto_explore) explore_step();` before running, `else if
  (explore_stairs) explore_stairs_arrive();` after travel; `dungeon()`
  calls `explore_new_level()` after `p_ptr->leaving = FALSE`; `disturb()`
  (`cave.c`) clears `auto_explore`; `cmsg_print()` (`message.c`) clears it
  on every new message (replaces Zangband's `message_num()` snapshot:
  Frog's repeated messages only bump a count). `do_cmd_go_up/down()` call
  `explore_to_stairs()` instead of "I see no ... staircase here".
- **Known grid**: `cave[y][x].info & CAVE_MARK`, plus in the dungeon the
  explorer's own `explore_seen[][]` (known must not shrink). Features as
  the player sees them: `f_info[get_feat_mimic(c_ptr)]` flags.
- Targets: known grid next to an unknown one, or a found object
  (`OM_FOUND`) not stood on yet (free bit `0x80000000` of `o_ptr->marked`
  = `OM_EXPLORED`). Avoids `FF_TRAP`, `FF_STORE`/`FF_BLDG`, `FF_LAVA`,
  `FF_ACID`, deep water, visible monsters; else `player_can_enter()`.
  Opens closed doors (`do_cmd_open_aux()`), never locked/jammed ones (true
  feature `power` / no `FF_OPEN`); digs rubble (`do_cmd_tunnel_aux()`, its
  "You dig" messages don't stop it). Stops: disturb, new message, visible
  non-pet/non-friendly monster in view, step that didn't move, no light
  (dungeon), confused/blind/hallucinating, nothing left ("Only locked doors
  or known traps are in the way." when that is why).
- Stairs: `explore_find()` in stairs mode (nearest known `FF_LESS`/`FF_MORE`,
  not `FF_QUEST_ENTER`) → **`travel_begin(TRAVEL_MODE_NORMAL, x, y)`**;
  when travel ends `explore_stairs_arrive()` takes them if stood on. A
  monster moving in view disturbs travel each turn (one step per press).
  Surface: explore/stairs search only the current town's wilderness
  square (`wilderness[..].town == p_ptr->town_num`, mapping from
  `_generate_cave()`); `<` on the surface and `>` in `wild_mode` keep
  their world-map toggle.
- **auto_more**: `message.c` `msg_line_flush()` never waits at `-more-`
  under `USE_WEB` (Frog has no option; `auto_more_state` is transient).
  Prompts (`msg_prompt`, `get_check`) still wait. Birth had no `-more-`.
- Help: `lib/help/command.txt` (X), `commdesc.txt` (Auto-explore, `<`/`>`).
- `sound.cfg` is now in the preload (`/frogcomposband/lib/xtra/sound/`),
  read lazily by `loadSoundCfg()` in `web/frogcomposband.js` (a fetch of
  `.cfg` was served as octet-stream = download prompt in the pane).
- Tested in the browser (own tab, 127.0.0.1): town `X` ("Nothing left"),
  `>` in town walks to the entrance and asks; dungeon: explore over many
  presses (rooms, corridors, doors, rubble, items, trap stop, monster
  stops, locked-door message), `>` walked to a known `>` and descended,
  Warrens L1 `<` walked to `<` and went up; `X` on the world map does
  nothing. Beginner mode = coffee-break: **no up staircases** (shafts), use
  Normal game speed to test `<`. Test IDBFS databases deleted.
- ASan (native, pty, random keys + 40x `X`, 15x `<`/`>`): 7 seeds, explore
  messages seen; two upstream bugs fixed in `port:` commit `28c0daf2`
  (`autopick.c` `insert_macro_line()` key burst, `cmd4.c` knowledge
  monsters visual mode on an empty group → `r_info[-1]`), then clean.
- Open problems: explore stops each time it walks over an item it already
  visited ("You see ..." message); a gap in the known map (after teleport)
  gives "You know of no way down" although a `>` is visible; reloading the
  page without quitting leaves temp floor files → "old temporal files"
  y/n at start (stage 5, save on unload?); no explore key in the roguelike
  keyset.

### Next: stage 3 (Enter menu + inventory)
- Keys: `request_command()` (`util.c` ~l.3600): Enter / `x` open Frog's
  own command menu `inkey_from_menu()` (`util.c` ~l.3414, tables
  `menu_info[10][10]` + `special_menu_info[]`, option `command_menu`
  default on); `pref-key.prf` has `C:0:^J` → `\r`. Then
  `process_command()` switch in `dungeon.c`. Add `X` (explore) to the menu.
- Template: Zangband's stage 3 (`~/Games/zangband/HANDOVER.md` "Stage 3").
  Frog's own UI code: `menu.c`/`menu.h`, item prompts `obj_prompt.c`
  (`obj_prompt()`), `inv.c`, `equip.c`, `quiver.c`; item commands in
  `cmd3.c` (wear/drop/inspect), `cmd6.c` (eat/quaff/read/use), `cmd5.c`
  (cast).

### Stage 3 (Enter menu + inventory): done 2026-09-26
- **Enter menu**: Frog's own `inkey_from_menu()` in `src/util.c` rewritten
  as Zangband's `cmd_menu()` (the fixed 2-column `menu_info[10][10]` /
  `special_menu_info[]` boxes are gone). Same call site in
  `request_command()`: Enter / `x` (when `command_menu` is on and no keymap
  uses the key; `pref-key.prf` `^J` → `\r` opens it too). Groups and
  commands in `cmd_menu_list[]` as `lib/help/commdesc.txt` groups them (14
  groups, incl. `X` explore, `<`/`>`, travel, pref/Mogaminator, `^I`).
  Boxes `box_draw()` / `box_menu()` (sized to content, no scrolling);
  keys of the current keyset by `command_key()` / `command_key_str()`
  (reverse lookup in `keymap_act`; roguelike shows `^D`, `T`, `,`, `^E`;
  explore and `W` have no roguelike key, pick them by cursor). 2/8/arrows
  move, Enter/Space/5/6/right choose, group letter or command key chooses,
  Esc/0/4/left back. The chosen underlying command skips the keymaps
  (`inkey_next = ""`), runs through `process_command()`.
- **Item menus** (`i`/`e` = `gear_ui()` in `src/obj.c`, Frog's
  `obj_prompt()` with the new `_gear_handler()` instead of `_inspector`):
  letter = main action (`_gear_main()`: eat, quaff, read, use, aim, zap,
  cast, wear, take off, refuel, else examine), Shift+letter drop,
  Ctrl+letter examine, Enter/Space/5 = `_gear_menu()` (a `box_menu()` of
  every action in `_gear_act[]` that fits: the same item test and places as
  the command's own `obj_prompt()`; `obj_can_eat/quaff/read()` wrappers in
  `cmd6.c`, `obj_can_wield()` in `equip.c`), `+ - *` = main/drop/examine
  of the cursor's item.
- **Cursor in every item prompt** (`src/obj_prompt.c`): `context.cursor`
  (new field in `obj_prompt.h`), `_cursor_move()` / `_cursor_ok()`, drawn
  as `>` by `inv_display()` (`inv_display_cursor`, `inv.c`). 2/8/arrows
  move, 4/6/arrows switch tab, Enter/5 choose (labels and `@` tags first,
  so `@5` still works). Esc cancels; Enter without a cursor still cancels.
- **How item actions run (queue + preselect via obj_prompt)**: the handler
  sets `obj_prompt_preselect = obj`, `queue_raw_command(key)` (util.c:
  `command_new` + `command_raw` → no keymap) and `gear_reopen = 'i'/'e'`,
  then dismisses. The command runs through `process_command()`; its first
  `obj_prompt()` takes the preselect (`_preselect()`: if a tab offers the
  object it returns it, or for handler prompts like inspect/inscribe calls
  the handler with the object's label) and always clears it. `dungeon.c`
  normal-command branch: clears the preselect after a command unless one
  is queued; before `request_command()` queues `gear_reopen` unless
  `hostile_in_view()` (new in `cmd2.c`, also used by explore).
- Tested in the browser (own tab, 127.0.0.1, Beginner Hobbit Rogue): Enter
  menu 14 groups, cursor pick (Game status → time), letter/key pick
  (`b` `X` explore), Esc closes; items: eat (letter), read (Recall letter;
  Teleportation via menu `r`), quaff (Potion of Sight bought at the
  Alchemist, letter), wield (letter, slot menu), take off (letter in `e`),
  drop (Shift, quantity prompt), examine (menu `I`, Ctrl+b); list reopens
  after each. Roguelike (`!` `Y:rogue_like_commands`): menu shows roguelike
  keys, item menu take off by `T`, wield by letter, explore from the menu.
  Test IDBFS databases deleted.
- ASan (native, pty, random keys weighted to Enter, `i`/`e`, letters,
  Shift/Ctrl letters, 2/4/6/8, arrows, `X`, `<`/`>`; 4 seeds x (3000 new +
  2000 restored)): menus drawn 8–73 times per run, no reports.
- Open problems: the reopened list hides the action's message (it is in
  `^P`); Tab/^E/^P/^Q/^F/^W in `i`/`e` now examine an item instead of
  switching tab/toggling (use 4/6 or `/`); the cursor starts at the top
  after the list reopens; other keys in the list are ignored (as before),
  not run as commands; the item menu box can cover the list's right part.

### Next: stage 4 (tiles)
- Decision from stage 1: **Shockbolt** (own 16x16 covers only 59.4% of
  r/k/f_info, `python3 web/tile-coverage.py`; below 95%).
- Template: Zangband's Stage 4 (`~/Games/zangband/HANDOVER.md`):
  `web/mkgraf-shb.py` writes `lib/pref/graf-shb.prf` (names + family
  stand-ins), own mode `GRAPHICS_SHOCKBOLT` (`$GRAF` "shb"), sheet
  `web/tiles.webp` from `~/Games/tactical-angband`, drawn by `js_pict` /
  `qb.pict` in `web/frogcomposband.js`, big-tile mode. Frog's `graf-new.prf`
  uses `K:tval:sval`: count and map objects by their `I:` line.

### Stage 4 (tiles): done 2026-09-26
- **Tile set: Shockbolt** 64x64 (Raymond Gaustadnes; Angband 4.2
  `lib/tiles/shockbolt/64x64.png`, copied as `web/tiles.webp` from
  `~/Games/tactical-angband/web/tiles.webp`, lossless, 13 MB, committed; the
  build copies it to `web/dist`). The one set (own 16x16 = 59.4%). Drawn
  nearest-neighbour at the map cell size in **big-tile mode** (one grid = two
  half-width cells, tile = `L.tile`, Zoom -/+).
- **Pref**: `lib/pref/graf-shb.prf`, generated by `python3 web/mkgraf-shb.py`
  (port of Zangband's) from tactical-angband's `graf-shb-dark.prf`,
  `flvr-shb.prf`, `xtra-shb.prf` and 4.2 `gamedata`. Monsters `R:idx` and
  objects `K:tval:sval` by name (aliases for Novice *, Grip/Fang), else
  family stand-ins (monster: same symbol + colour; object: first tile of the
  tval). Flavoured food/potions/scrolls (k_info `N:idx:name:flavour`) → 4.2
  flavour tile of the same name, else by colour. Books: Frog's 20 realms →
  4.2 prayer/magic/nature/shadow books by sval. Features `F:idx:lit:torch:dark`
  (Frog's standard/lite/dark triplet = Shockbolt's lit/torch/dark, no C shift
  needed), all 188 by f_info tag by hand (shops → 4.2 `STORE_*`, buildings →
  door tile, traps → 4.2 traps). `S:0x30..0x7F` bolts. Player `R:0` per
  class/race from `xtra-shb.prf` (no `$GENDER` lines). Loaded via
  `graf-x11.prf` `?:[EQU $GRAF shb]`.
- **Coverage** (`python3 web/tile-coverage.py`; `... new` = own 16x16 set):
  **2117/2119 = 99.9%** (r_info 1395/1395, k_info 536/536, f_info 186/188;
  the 2 misses are NONE/UNDETECTED = unknown grid, an empty tile). 681 by
  name, 374 by hand, **1063 family stand-ins** (929 monsters, 134 objects).
- **C (decides per cell)**: `GRAPHICS_SHOCKBOLT 3` (`defines.h`);
  `src/main-web.c` `web_graphics()` sets `use_graphics`, `arg_graphics`,
  `ANGBAND_GRAF` "shb", `arg_bigtile` from `js_tiles_wanted()` in
  `init_web()`; `web_switch_graphics()` (Term_resize of term 0 to take
  `arg_bigtile`, `reset_visuals()`, `do_cmd_redraw()`) when the page's
  button changed, at the command prompt only (`web_pump()`).
  `Term_pict_web()` passes `big` = next cell is the `AF_BIGTILE2` pad (map
  tiles 2 cells wide; item tiles in lists/sidebar 1 cell). `cave.c`
  `map_info()` (USE_WEB): `FF_PLANT` features (trees, flowers, brake) get
  grass as background (Shockbolt trees are cut-outs).
- **Big-tile fix (upstream Frog had it broken)**: Frog's viewport code
  (`xtra2.c` `cave_pt_to_ui_pt()`/`ui_pt_to_cave_pt()`) ignored bigtile. New
  `UI_MAP_STEP` (`externs.h`, 2 in big-tile mode): x doubled/halved in the
  point conversions; `cave_pt_is_visible()` needs both cells; grid counts
  `r.cx / UI_MAP_STEP` in `viewport_scroll()`, `viewport_verify_aux()`,
  target/look functions (`xtra2.c`), `do_cmd_locate()` (`cmd3.c`),
  `_scroll_panel()` (`wild.c`); `prt_map()` and `target_set_prepare()` step
  by `UI_MAP_STEP`. Inert (step 1) in text mode / native builds.
- **JS only blits**: `web/frogcomposband.js` `pict(..., big)` (terrain tile,
  then the tile over it), `tilesWanted`/`tilesSwitch`; **Tiles: on/off**
  button (`btn-tiles` in `web/index.html`), saved in the layout file as
  `L.text`. No `.prf` fetch (prefs are in the preload).
- Tested in the browser (own tab, 127.0.0.1): town (shop sign tiles, brick
  buildings, doors, grass, trees on grass, water, entrance), dungeon L2/L4/L6
  via `>` and recall + `X` (granite/magma/treasure veins, open/closed doors,
  trap door trap, creeping coins, Fang, snake, orcs, dagger/corpse items),
  hobbit rogue hero tile, `l` look, `L` locate (half-panel scroll), `M`
  reduced map, inventory list item tiles. Canvas check after `^R`: 630 pict
  cells, 627 with only tile-sheet colours (exact, nearest-neighbour; 3 alpha
  edges), all map tiles `big`; text mode: 0 pict calls. Tiles→text→tiles
  three times, map intact (same cell counts); Tiles off survives a reload.
  Test IDBFS databases (`/frogcomposband/lib/*`) deleted.
- Native ASan with graphics: skipped (native build is curses `USE_GCU`, no
  graphics; `UI_MAP_STEP` is 1 there, the `map_info()` change is web only).
- Open problems: 1063 stand-ins (Frog-only monsters share family tiles);
  bookstore uses 4.2's STORE_BOOK tile (labelled "Temple"); swamp/slush/
  toxic waste use the water tile, snow the sand tile, glacier/ice walls
  quartz; lighting shades only with `view_special_lite`-style options; the
  reduced map (`M`) squashes tiles; item tiles in lists are 1 cell (half
  width, as X11); Beginner mode has no stairs (shafts) to eyeball, town
  entrance uses the down-stair tile. Credit Shockbolt in Help/README
  (stage 6/7).

### Next: stage 5 (web page)
- Windows: Inventory/Visible/Messages are still empty (Stage 1 problem: no
  window flags) → set them in `init_web()` from one table like Zangband's
  `web_window_flags[]` (`~/Games/zangband/src/main-web.c`, RVIP W4); check
  `birth.c`-style defaults that overwrite them.
- Reload without quitting → "There are old temporal files ... delete? [y/n]"
  at start (Stage 2 problem): answer/avoid it in the web build (the temp
  floor files live in `lib/save`, IDBFS).
- Layout file `/frogcomposband/lib/user/web-layout.json` (already holds
  splits, zoom, audio, `text`); game end via `quit_aux`/`js_quit` as
  Zangband (death → tombstone key waits → reload into a new birth).
- `web/deploy.sh` from Zangband's, target `ruzzoli.de/roguelikes/frogcomposband/`;
  no deploy until stage 7.

### Stage 5 (web page): done 2026-09-26
- **Windows** (`rvip-wm.js`, shared copy; `web/index.html` `#t-<id>`,
  `TERMS` in `web/frogcomposband.js`, 8 terms = `WEB_TERMS` in
  `src/main-web.c`, the z-term maximum): 0 Map, 1 Inventory `PW_INVEN`,
  2 Messages `PW_MESSAGE`, 3 Visible `PW_MONSTER_LIST`, 4 Recall
  `PW_MONSTER|PW_OBJECT`, 5 Equipment `PW_EQUIP`, 6 Objects
  `PW_OBJECT_LIST`, 7 Character `PW_PLAYER` (new: `defines.h` 0x1000,
  `window_flag_desc[12]` "Display character" in `tables.c` so the window
  mask accepts it; `fix_player()` in `py_info.c` draws page 1 of the
  character sheet, one column below 80 cols; `redraw_stuff()` in `xtra1.c`
  sets it on stat/HP/level/gold/equippy redraws). Flags from
  `web_window_flags[]`, set in `init_web()` (before load; birth doesn't
  touch them; a savefile brings its own). Default on: Map, Inventory,
  Visible, Messages; Recall, Equipment, Objects, Character via Windows.
  Spell list `PW_SPELL` is a no-op upstream (`fix_spell()` body commented
  out, no desc): no window.
- **Layout file** `/frogcomposband/lib/user/web-layout.json` (IDBFS; splits,
  wm tree incl. which windows are on, zoom, fonts, titles, Tiles, audio).
- **Temp-files prompt fixed**: `floors.c` `init_saved_floors()` forces
  `force = TRUE` under `USE_WEB` (the leftover `0.PLAYER.Fnn` files of a
  reload are deleted silently; one game per tab).
- **Game end**: one path, `play_game()` end → `close_game()` (`files.c`:
  death → `print_tomb()` key wait (RET = character sheet `show_info()`, ESC
  skips) → scores) → `quit(NULL)` → `quit_aux` = `hook_quit` (set in
  `init_web()`, after main.c's own) → `js_quit(msg, p_ptr->is_dead)`.
  Dead: page syncs and reloads into a new birth. Ctrl-X: saves, "Play again"
  overlay (reload restores). The only `exit()` calls are in `z-util.c`
  `quit()`, after `quit_aux`.
- **Help**: `build.sh` writes a stub `help.html` (stage 6 replaces it).
- **`web/deploy.sh`**: Zangband's, target `ruzzoli.de/roguelikes/frogcomposband/`.
  Dry run: "commit + push first", exit 1. **Not deployed, no repo.**
- Tested (own tab, 127.0.0.1, 1440x900): new Beginner character → inventory,
  messages, equipment, objects (town features, then floor items), character
  sheet, white icky thing in Visible and Recall; gutter drag + 4 extra
  windows on → reload → same layout; reload in the dungeon with `.F00` on
  disk → no temp-files prompt; Ctrl-X → overlay → Play again → restored;
  suicide (`Q y @`) → tombstone → ESC → new birth, and again with RET →
  character sheet → ESC → new birth, windows right; Help opens/Esc closes;
  no console errors. IDBFS `/frogcomposband/lib/*` deleted afterwards.
  Native ASan (1 seed, 2500 new + 1500 restored keys): clean.
- Open problems: no high-score list after death (Frog's `close_game()`
  shows none, upstream); Character window shows only page 1 of the sheet;
  message history of a dead character may carry into the new one; 13 MB
  `tiles.webp` loads slowly; Ctrl-X quits with no "Press Return".

### Next: stage 6 (docs + sound)
- Sound: `sound.cfg` is already in the preload, read by `loadSoundCfg()`
  (`web/frogcomposband.js`); `TERM_XTRA_SOUND` → `js_sound` hook in
  `main-web.c`. Copy `~/Games/zangband/web/sounds.py` (Dubtrain wavs).
  Sound/Music buttons exist, off by default. Commit the music file under
  `web/music` (build.sh copies it from `../quickband/web/music/new_town.ogg`
  now).
- Help: `web/make-help.py` from Zangband's with `PAGE='frogcomposband.html'`,
  `build.sh` writes `$OUT/help.html` from it (replace the stub line).
- Docs entry under `~/Desktop/Games/Roguelikes/Docs/` with both keysets
  (explore `X` original only; Enter menu), Tips section, Shockbolt credit.

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

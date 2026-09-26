**RVIP port** of FrogComposband 7.1.salmiak.6 (2023), upstream
[sulkasormi/frogcomposband branch `master` @ `3d28f6b1`](https://github.com/sulkasormi/frogcomposband/tree/3d28f6b1)
(Antero Sulka). Play: https://ruzzoli.de/roguelikes/frogcomposband/
Our changes: https://github.com/memmaker/frogcomposband/compare/3d28f6b1...master

Lineage: Moria → Angband (1990) → Zangband (Topi Ylinen, 1994) → Hengband
(Mr.Hoge and others) → Chengband (Chris Kousky, Dave Zhang, Andrew Levy) →
PosChengband (Chris Kousky, with ideas from Posband) → ComPosband (Gwilim
Owen) → FrogComposband (Antero Sulka, first beta 2018). The upstream notes
are in `readme.txt`, `lib/help/` (`version.txt`, `vers_old.txt`) and
`lib/file/credits.txt`.

What this port adds (the game code in `src/` is nearly untouched):
- **Web frontend** `src/main-web.c` (z-term, from TinyAngband) and `web/`
  (`frogcomposband.js`, shared `rvip-wm.js`): Emscripten + Asyncify, saves in
  the browser's IndexedDB.
- **Windows**: main map plus Inventory, Visible, Messages, Recall,
  Equipment, Objects and a new Character window; drag bars to resize,
  titles to move, layout via Windows ▾ (kept across reloads).
- **Tiles**: Shockbolt mode (Angband 4.2's 64×64 Shockbolt set, mapping in
  `lib/pref/graf-shb.prf` from `web/mkgraf-shb.py`), drawn nearest-neighbour;
  upstream bigtile viewport fix.
- **Explore** `X`: walks to the nearest unexplored spot, stops for monsters
  and messages. **`<` / `>`** take the stairs you stand on, or travel to the
  nearest known ones (end of `src/cmd2.c`, through Frog's own travel code;
  `` ` `` travel and `J` resume work as upstream).
- **Enter menu**: every command, grouped as the help's command list, with
  the key of the current keyset (`cmd_menu()` in `src/util.c`).
- **Item menus**: `i` / `e` show a cursor list; Enter opens the actions for
  the item, letter = main action, Shift = drop, Ctrl = examine (`gear_ui()`
  in `src/obj.c`); every item prompt has a cursor (`src/obj_prompt.c`).
- **Sound** (off by default): Dubtrain effects (`web/sounds.py`) and a town
  music loop (`web/music/new_town.ogg`, from Quickband).
- No `-more-` prompts in the browser; fixes for upstream memory bugs found
  with ASan (commits `5b0104a5`, `28c0daf2`).

Controls: the original keyset (or the roguelike one, `=` options); Enter for
the command menu, `X` explore, `<`/`>` stairs, `?` help. New game: `b`
(Beginner), Enter, Enter is a quick start. The Help button opens the game
guide.

Build: `sh web/build.sh` → `web/dist` (needs emcc, cc, python3).
Deploy: `sh web/deploy.sh`. Notes: `HANDOVER.md`.

Credits: FrogComposband by Antero Sulka (sulkasormi) and contributors;
ComPosband by Gwilim Owen; PosChengband by Chris Kousky; Chengband by Chris
Kousky, Dave Zhang and Andrew Levy; ideas from Posband (Alexander Ulyanov,
Graham Philips); Hengband by Mr.Hoge and many others; Zangband by Topi
Ylinen, Robert Ruehlmann and the ZAngband DevTeam; Angband by Ben Harrison
and others, from Moria/Umoria (Robert Alan Koeneke, James E. Wilson); see
`lib/file/credits.txt`. Licence: the Angband/Moria notice in the source
headers (some newer files dual GPL 2 / Angband). Shockbolt tiles © Raymond
Gaustadnes (Shockbolt) 2012. Sound effects: Dubtrain sound pack.

#!/bin/sh
# Build FrogComposband for the browser (Emscripten + Asyncify).
# Output goes to web/dist; deploy with web/deploy.sh.
set -e
cd "$(dirname "$0")/.."
OUT=web/dist
rm -rf "$OUT" web/stage && mkdir -p "$OUT" web/stage/lib

# Game files (no X11 fonts, BMP tiles)
for d in edit file help pref; do cp -R lib/$d web/stage/lib/; done
# Sound: sound.cfg (FrogComposband event names) + Dubtrain wavs, written by web/sounds.py
mkdir -p web/stage/lib/xtra/sound && python3 web/sounds.py web/stage/lib/xtra/sound/sound.cfg "$OUT/sound"
mkdir -p web/stage/lib/data web/stage/lib/info web/stage/lib/save web/stage/lib/user web/stage/lib/apex web/stage/lib/bone web/stage/lib/script
find web/stage \( -name 'Makefile*' -o -name 'delete.me' -o -name '*.vim' -o -name '*.sh' \) -delete

# Sources: CFILES/ZFILES/ANGFILES of Makefile.src (the autotools list), no main-*
SRCS=$(tr -d '\r' < src/Makefile.src | sed -n '/^CFILES/p;/^ZFILES/p;/^ANGFILES/,/^$/p' \
	| grep -o '[a-z0-9_-]*\.o' | sed 's/\.o$/.c/;s|^|src/|' | sort -u)

emcc -O2 -fcommon -std=gnu99 -DUSE_WEB -Isrc -w \
	-DDEFAULT_CONFIG_PATH='"./lib/"' -DDEFAULT_LIB_PATH='"./lib/"' -DDEFAULT_DATA_PATH='"./lib/"' \
	$SRCS src/main.c src/main-web.c \
	-o "$OUT/frogcomposband-core.js" \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=64MB \
	-sEXPORTED_FUNCTIONS=_main,_web_request_save \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,addRunDependency,removeRunDependency \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web \
	--preload-file web/stage/lib@/frogcomposband/lib

cp web/index.html web/frogcomposband.js "$OUT/"
# Shockbolt tiles (Angband 4.2 lib/tiles/shockbolt/64x64.png), lossless WebP, as in
# ~/Games/tactical-angband/web/tiles.webp
cp web/tiles.webp "$OUT/"
# Font choosers: the index page's fonts/*.woff (loaded from ../fonts/)
FONTS="${FONTS:-$HOME/Games/roguelikes-index/fonts}"
(ls "$FONTS" 2>/dev/null | sed -n 's/\.woff$//p') | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read().split()))' > "$OUT/fonts.json"
# Help: the game guide from ~/Desktop/Games/Roguelikes/Docs
python3 web/make-help.py > "$OUT/help.html"
# Town music (depth 0), vendored from Quickband
mkdir -p "$OUT/music" && cp web/music/new_town.ogg "$OUT/music/"
rm -rf web/stage
ls -la "$OUT"

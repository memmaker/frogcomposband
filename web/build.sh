#!/bin/sh
# Build FrogComposband for the browser (Emscripten + Asyncify).
# Output goes to web/dist; deploy with web/deploy.sh.
set -e
cd "$(dirname "$0")/.."
OUT=web/dist
rm -rf "$OUT" web/stage && mkdir -p "$OUT" web/stage/lib

# Game files (no X11 fonts, BMP tiles)
for d in edit file help pref; do cp -R lib/$d web/stage/lib/; done
mkdir -p web/stage/lib/xtra/sound && cp lib/xtra/sound/sound.cfg web/stage/lib/xtra/sound/
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

cp web/index.html "$HOME/Games/rvip-tools/web/rvip-wm.js" web/frogcomposband.js "$OUT/"
# Town music is stage 6 (sound.cfg is in the preload)
mkdir -p "$OUT/music" && cp ../quickband/web/music/new_town.ogg "$OUT/music/"
rm -rf web/stage
ls -la "$OUT"

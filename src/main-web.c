/* File: main-web.c */

/*
 * Browser (Emscripten/WASM) front end for FrogComposband.
 *
 * All drawing is done by JavaScript on one <canvas> per term (see
 * web/frogcomposband.js).  Blocking input uses Asyncify: when the game waits
 * for a key we sleep in emscripten_sleep(), which yields to the browser.
 *
 * The module registers itself as "x11" so that the same pref files
 * (keymaps, window layout, graphics) as the X11 build are used; special
 * keys are sent in the X11 keysym macro format.
 */

#include "angband.h"

#ifdef USE_WEB

#include <emscripten.h>

#define WEB_TERMS 8		/* terms 1-7: see web_window_flags[] */

static term web_term[WEB_TERMS];

/* Pending "save now" request from the page (tab hidden / closing) */
static int web_want_save = 0;

/* Last time we yielded to the browser */
static double web_last_yield = 0;


/* ---- JavaScript side (implemented in web/frogcomposband.js) ---- */

EM_JS(void, js_text, (int t, int x, int y, int n, int a, const char *s), {
	Module.qb.text(t, x, y, n, a, s);
});

EM_JS(void, js_wipe, (int t, int x, int y, int n), {
	Module.qb.wipe(t, x, y, n);
});

EM_JS(void, js_clear, (int t), {
	Module.qb.clear(t);
});

EM_JS(void, js_curs, (int t, int x, int y, int w), {
	Module.qb.curs(t, x, y, w);
});

/* big = 1 in big-tile mode: the tile covers this cell and the next one */
EM_JS(void, js_pict, (int t, int x, int y, int n, const byte *ap, const char *cp,
                      const byte *tap, const char *tcp, int big), {
	Module.qb.pict(t, x, y, n, ap, cp, tap, tcp, big);
});

/* Tiles (1) or text (0) as the page's Tiles button says; -1: no change */
EM_JS(int, js_tiles_wanted, (void), {
	return Module.qb.tilesWanted();
});

EM_JS(int, js_tiles_switch, (void), {
	return Module.qb.tilesSwitch();
});

EM_JS(void, js_fresh, (int t), {
	Module.qb.fresh(t);
});

EM_JS(void, js_bell, (void), {
	Module.qb.bell();
});

EM_JS(void, js_sound, (const char *name), {
	Module.qb.sound(UTF8ToString(name));
});

EM_JS(void, js_depth, (int depth), {
	Module.qb.depth(depth);
});

EM_JS(void, js_color, (int i, int r, int g, int b), {
	Module.qb.color(i, r, g, b);
});

EM_JS(int, js_term_cols, (int t), {
	return Module.qb.termCols(t);
});

EM_JS(int, js_term_rows, (int t), {
	return Module.qb.termRows(t);
});

/* Layout changes after a browser resize */
EM_JS(int, js_layout_pending, (int t), {
	return Module.qb.layoutPending(t);
});

EM_JS(int, js_pending_cols, (int t), {
	return Module.qb.pendingCols(t);
});

EM_JS(int, js_pending_rows, (int t), {
	return Module.qb.pendingRows(t);
});

EM_JS(void, js_apply_layout, (int t, int cols, int rows), {
	Module.qb.applyLayout(t, cols, rows);
});

/* Next queued input: -1 none, else key; mouse events via js_mouse_* */
EM_JS(int, js_next_event, (int at_cmd), {
	return Module.qb.nextEvent(at_cmd);
});


EM_JS(void, js_quit, (const char *msg, int dead), {
	Module.qb.quit(msg ? UTF8ToString(msg) : "", dead);
});

EM_JS(void, js_plog, (const char *msg), {
	Module.qb.plog(UTF8ToString(msg));
});

EM_JS(void, js_sync, (void), {
	Module.qb.sync();
});


/* Graveyard + leaderboard beacon (roguelikes-index/server/CONTRACT.md) */
EM_JS(void, js_beacon, (const char *g, const char *ev, const char *name, const char *killer, int depth, int score, int turns, int lvl), {
	try {
		var p = [['g', UTF8ToString(g)], ['ev', UTF8ToString(ev)], ['name', name ? UTF8ToString(name) : ''],
		         ['killer', killer ? UTF8ToString(killer) : ''], ['depth', depth], ['score', score], ['turns', turns], ['lvl', lvl]];
		var q = p.filter(function (a) { return a[1] !== '' && !(a[1] < 0); })
		         .map(function (a) { return a[0] + '=' + encodeURIComponent(a[1]); }).join('&');
		if (window.RvipWM && RvipWM.report) RvipWM.report(q); else fetch('/roguelikes/beacon?' + q, { keepalive: true, mode: 'no-cors' }).catch(function () {});
	} catch (e) {}
});

/* Called from close_game() (files.c) first thing when the run is over,
   before kingly()/tombstone; suicide and signals also die ("Quitting",
   "Interrupting", "Abortion"); a won run retiring is still a win. */
void web_run_end(void)
{
	cptr k = p_ptr->died_from, ev = "death";
	static char b[80];

	if (p_ptr->total_winner) ev = "win", k = NULL;
	else if (streq(k, "Quitting") || streq(k, "Interrupting") || streq(k, "Abortion")) ev = "quit", k = NULL;
	else if (prefix(k, "a ")) k += 2;
	else if (prefix(k, "an ")) k += 3;
	else if (prefix(k, "the ") || prefix(k, "The ")) k += 4;
	if (k && suffix(k, " while helpless"))	/* effects.c take_hit() */
	{
		my_strcpy(b, k, sizeof b);
		b[strlen(b) - 15] = 0;
		k = b;
	}
	js_beacon("frogcomposband", ev, player_name, k, dun_level, (int)hof_score(), (int)turn_real(game_turn), p_ptr->lev);
}


/* Persist the save directories (called after every save) */
void web_sync_files(void)
{
	js_sync();
}


/* Called from JS when the page is about to be hidden or closed */
EMSCRIPTEN_KEEPALIVE void web_request_save(void)
{
	web_want_save = 1;
}


/*
 * Resize the terms to the layout the page computed after a browser resize.
 * Term_resize() runs the resize hooks (resize_map / redraw_window), which
 * redraw the contents.  The main window changes its size only at the
 * command prompt; a cell-size change alone applies at once.
 */
static void web_apply_layout(void)
{
	int i;
	term *old = Term;
	bool at_prompt = (inkey_flag && character_generated);

	for (i = 0; i < WEB_TERMS; i++)
	{
		term *t = &web_term[i];
		int cols, rows;

		if (!js_layout_pending(i)) continue;

		cols = js_pending_cols(i);
		rows = js_pending_rows(i);
		if (cols < 1) cols = 1;
		if (rows < 1) rows = 1;

		if (!i)
		{
			if (cols < 80) cols = 80;
			if (rows < 24) rows = 24;

			if (((cols != t->wid) || (rows != t->hgt)) && !at_prompt) continue;
		}

		/* New canvas size and cell size (the canvas starts blank) */
		js_apply_layout(i, cols, rows);

		Term_activate(t);
		if ((cols == t->wid) && (rows == t->hgt)) Term_redraw();
		else Term_resize(cols, rows);
	}

	Term_activate(old);
}


static void web_switch_graphics(int on);

/* Move queued browser input into the main term's key queue */
static int web_pump(void)
{
	int k, got = 0;
	term *old = Term;

	web_apply_layout();

	Term_activate(&web_term[0]);

	while ((k = js_next_event(inkey_flag && character_generated)) >= 0)
	{
		/* No mouse support in this variant */
		if (k != 0x10000) Term_keypress(k);
		got = 1;
	}

	/* Tiles <-> text: only while waiting for a command */
	if (inkey_flag && character_generated && !got)
	{
		int on = js_tiles_switch();

		if ((on >= 0) && (on != (use_graphics != GRAPHICS_NONE)))
		{
			web_switch_graphics(on);
			got = 1;
		}
	}

	/* Safe autosave: only while waiting for a command */
	if (web_want_save && inkey_flag && character_generated &&
	    !p_ptr->is_dead && !got && (Term->key_head == Term->key_tail))
	{
		web_want_save = 0;
		Term_keypress(KTRL('S'));
		got = 1;
	}

	Term_activate(old);
	return got;
}

static void web_yield(int ms)
{
	emscripten_sleep(ms);
	web_last_yield = emscripten_get_now();
}

static errr web_check_events(int wait)
{
	if (web_pump()) return (0);

	if (!wait)
	{
		/* Let the browser paint now and then during long actions */
		if (emscripten_get_now() - web_last_yield > 50) web_yield(0);
		return (web_pump() ? 0 : 1);
	}

	while (1)
	{
		web_yield(10);
		if (web_pump()) return (0);
	}
}

static void web_react(void)
{
	int i;

	for (i = 0; i < 16; i++)
		js_color(i, angband_color_table[i][1], angband_color_table[i][2],
		         angband_color_table[i][3]);
}

static int web_idx(void)
{
	return (int)(Term - web_term);
}

static errr Term_xtra_web(int n, int v)
{
	switch (n)
	{
		case TERM_XTRA_NOISE: js_bell(); return (0);
		case TERM_XTRA_SOUND:
			if ((v > 0) && (v < SOUND_MAX)) js_sound(angband_sound_name[v]);
			return (0);
		case TERM_XTRA_FRESH:
			js_fresh(web_idx());

			/* The page's Sound button is the only switch (off by default) */
			use_sound = TRUE;

			/* The page plays town music at depth 0 */
			js_depth(character_generated ? dun_level : -1);
			return (0);
		case TERM_XTRA_BORED: return (web_check_events(0));
		case TERM_XTRA_EVENT: return (web_check_events(v));
		case TERM_XTRA_FLUSH:
			while (js_next_event(0) >= 0) ;
			return (0);
		case TERM_XTRA_CLEAR: js_clear(web_idx()); return (0);
		case TERM_XTRA_DELAY:
			js_fresh(web_idx());
			if (v > 0) web_yield(v);
			return (0);
		case TERM_XTRA_REACT: web_react(); return (0);
	}

	return (1);
}

/* No cursor box on the hero: the tile already shows where the player is */
static bool web_curs_on_hero(int x, int y)
{
	point_t ui;

	if (web_idx() || !character_generated || !character_dungeon) return FALSE;
	ui = cave_xy_to_ui_pt(px, py);
	return (ui.x == x) && (ui.y == y);
}

static errr Term_curs_web(int x, int y)
{
	if (!web_curs_on_hero(x, y)) js_curs(web_idx(), x, y, 1);
	return (0);
}

static errr Term_bigcurs_web(int x, int y)
{
	if (!web_curs_on_hero(x, y)) js_curs(web_idx(), x, y, 2);
	return (0);
}

static errr Term_wipe_web(int x, int y, int n)
{
	js_wipe(web_idx(), x, y, n);
	return (0);
}

static errr Term_text_web(int x, int y, int n, byte a, cptr s)
{
	js_text(web_idx(), x, y, n, a, s);
	return (0);
}

static errr Term_pict_web(int x, int y, int n, const byte *ap, const char *cp,
                          const byte *tap, const char *tcp)
{
	/* A map tile in big-tile mode: the next cell holds the pad (AF_BIGTILE2) */
	int big = use_bigtile && (x + 1 < Term->wid) &&
	          (Term->scr->a[y][x + 1] == 0xF0) && ((byte)Term->scr->c[y][x + 1] == 0xFF);

	js_pict(web_idx(), x, y, n, ap, cp, tap, tcp, big);
	return (0);
}


/* Shockbolt tiles (lib/pref/graf-shb.prf) in big-tile mode, or text */
static void web_graphics(int on)
{
	use_graphics = on;
	arg_graphics = on ? GRAPHICS_SHOCKBOLT : GRAPHICS_NONE;
	ANGBAND_GRAF = on ? "shb" : "ascii";
	arg_bigtile = on;
}

/* The page's Tiles button, applied at the command prompt */
static void web_switch_graphics(int on)
{
	term *old = Term;

	web_graphics(on);
	Term_activate(&web_term[0]);
	Term_resize(Term->wid, Term->hgt);	/* takes arg_bigtile, remaps the panel */
	reset_visuals();
	do_cmd_redraw();
	Term_activate(old);
}


static void hook_plog(cptr str)
{
	if (str) js_plog(str);
}

static void hook_quit(cptr str)
{
	int i;


	for (i = 0; i < WEB_TERMS; i++) (void)term_nuke(&web_term[i]);

	/* After death the tombstone and scores already waited for a key */
	js_sync();
	js_quit(str, p_ptr->is_dead);
}

/*
 * What each sub-window shows (TERMS in web/frogcomposband.js).  Set here for
 * new characters; a savefile brings its own flags.
 */
static const u32b web_window_flags[WEB_TERMS] =
{
	0,
	PW_INVEN,					/* 1 Inventory */
	PW_MESSAGE,					/* 2 Messages */
	PW_MONSTER_LIST,			/* 3 Visible */
	PW_MONSTER | PW_OBJECT,		/* 4 Recall */
	PW_EQUIP,					/* 5 Equipment */
	PW_OBJECT_LIST,				/* 6 Objects */
	PW_PLAYER					/* 7 Character */
};


errr init_web(int argc, char **argv)
{
	int i;

	(void)argc;
	(void)argv;

	/*
	 * Web defaults (init_angband() copies o_norm into the option flags;
	 * savefiles keep the player's own choice)
	 */
	for (i = 0; option_info[i].o_desc; i++)
	{
		if (option_info[i].o_var == &center_player)
			option_info[i].o_norm = TRUE;
	}

	for (i = 0; i < WEB_TERMS; i++) window_flag[i] = web_window_flags[i];

	web_react();

	/* Shockbolt tiles unless the page says text */
	web_graphics(js_tiles_wanted());
	use_bigtile = arg_bigtile;

	for (i = 0; i < WEB_TERMS; i++)
	{
		term *t = &web_term[i];
		int cols = js_term_cols(i), rows = js_term_rows(i);

		if (!i)
		{
			if (cols < 80) cols = 80;
			if (rows < 24) rows = 24;
		}

		term_init(t, cols, rows, (i == 0) ? 1024 : 16);

		t->soft_cursor = TRUE;
		t->attr_blank = TERM_WHITE;
		t->char_blank = ' ';

		t->xtra_hook = Term_xtra_web;
		t->curs_hook = Term_curs_web;
		t->bigcurs_hook = Term_bigcurs_web;
		t->wipe_hook = Term_wipe_web;
		t->text_hook = Term_text_web;
		t->pict_hook = Term_pict_web;
		t->higher_pict = TRUE;

		Term_activate(t);
		angband_term[i] = t;
	}

	Term_activate(&web_term[0]);

	web_last_yield = emscripten_get_now();

	quit_aux = hook_quit;
	plog_aux = hook_plog;

	return (0);
}

#endif /* USE_WEB */

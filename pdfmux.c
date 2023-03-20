#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_cursor.h>

static xcb_connection_t* conn;
static xcb_screen_t* scr;
static xcb_window_t win;
static xcb_key_symbols_t* ksyms;
static xcb_cursor_context_t* cctx;
static xcb_cursor_t cursor_hand;
static xcb_cursor_t cursor_crosshair;

static void
die(const char* msg)
{
	fprintf(stderr, "%s\n", msg);
}

static xcb_atom_t
get_x11_atom(const char* name)
{
	xcb_atom_t atom;
	xcb_generic_error_t* error;
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t* reply;

	cookie = xcb_intern_atom(conn, 0, strlen(name), name);
	reply = xcb_intern_atom_reply(conn, cookie, &error);

	if (NULL != error) {
		fprintf(stderr, "xcb_intern_atom() failed with error code: %hhu",
		    error->error_code);
	}

	atom = reply->atom;
	free(reply);

	return atom;
}

static void
xwininit(void)
{
	const char *wm_class, *wm_name;

	xcb_atom_t _NET_WM_NAME, _NET_WM_STATE, _NET_WM_STATE_FULLSCREEN, _NET_WM_WINDOW_OPACITY;

	xcb_atom_t WM_PROTOCOLS, WM_DELETE_WINDOW;

	xcb_atom_t UTF8_STRING;

	uint8_t opacity[4];

	conn = xcb_connect(NULL, NULL);

	if (xcb_connection_has_error(conn)) {
		die("Cannot open display\n");
	}

	scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

	if (NULL == scr) {
		die("Can't get default screen.");
	}

	if (xcb_cursor_context_new(conn, scr, &cctx) != 0) {
		die("Cannot create cursor context");
	}

	cursor_hand = xcb_cursor_load_cursor(cctx, "fleur");
	cursor_crosshair = xcb_cursor_load_cursor(cctx, "crosshair");
	ksyms = xcb_key_symbols_alloc(conn);
	win = xcb_generate_id(conn);

	xcb_create_window_aux(
		conn, scr->root_depth, win, scr->root, 0, 0,
		800, 800, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		scr->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(const xcb_create_window_value_list_t []) {{
				.background_pixel = 0x0e0e0e,
				.event_mask = XCB_EVENT_MASK_EXPOSURE |
				XCB_EVENT_MASK_KEY_PRESS |
				XCB_EVENT_MASK_BUTTON_PRESS |
				XCB_EVENT_MASK_BUTTON_RELEASE |
				XCB_EVENT_MASK_POINTER_MOTION |
				XCB_EVENT_MASK_STRUCTURE_NOTIFY
			}}
		);

	_NET_WM_NAME = get_x11_atom("_NET_WM_NAME");
	UTF8_STRING = get_x11_atom("UTF8_STRING");
	wm_name = "pdfmux";

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    _NET_WM_NAME, UTF8_STRING, 8, strlen(wm_name), wm_name);

	wm_class = "pdfmux\0pdfmux\0";
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win, XCB_ATOM_WM_CLASS,
			    XCB_ATOM_STRING, 8, strlen(wm_class), wm_class);

	WM_PROTOCOLS = get_x11_atom("WM_PROTOCOLS");
	WM_DELETE_WINDOW = get_x11_atom("WM_DELETE_WINDOW");

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    WM_PROTOCOLS, XCB_ATOM_ATOM, 32, 1, &WM_DELETE_WINDOW);

	_NET_WM_WINDOW_OPACITY = get_x11_atom("_NET_WM_WINDOW_OPACITY");
	opacity[0] = opacity[1] = opacity[2] = opacity[3] = 0xff;

	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    _NET_WM_WINDOW_OPACITY, XCB_ATOM_CARDINAL, 32, 1, opacity);

	_NET_WM_STATE = get_x11_atom("_NET_WM_STATE");
	_NET_WM_STATE_FULLSCREEN = get_x11_atom("_NET_WM_STATE_FULLSCREEN");
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
			    _NET_WM_STATE, XCB_ATOM_ATOM, 32, 1, &_NET_WM_STATE_FULLSCREEN);

	xcb_change_window_attributes(conn, win, XCB_CW_CURSOR, &cursor_crosshair);
	xcb_map_window(conn, win);
	xcb_flush(conn);
}

int
main(void)
{
	xwininit();
	return 0;
}

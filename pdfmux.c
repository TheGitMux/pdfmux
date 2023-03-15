#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mupdf/fitz.h>
#include <vulkan/vulkan.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

static uint32_t *px, pc;
static uint32_t color;
static uint32_t width, height;
static xcb_connection_t* c;
static xcb_gcontext_t gc;
static xcb_image_t* image;
static xcb_screen_t* scrn;
static xcb_window_t window;

static int page_count, page_number = 0;

static fz_context* ctx;
static fz_document* doc;
static fz_matrix ctm;
static fz_page* page;
static fz_pixmap* pix;

static void fz_cleanup(fz_context* ctx, fz_page* page, fz_document* doc, fz_pixmap* pix);
static void create_window(void);
static void delete_window(void);

static void
fz_cleanup(fz_context* ctx, fz_page* page, fz_document* doc, fz_pixmap* pix)
{
	fz_drop_pixmap(ctx, pix);
	fz_drop_page(ctx, page);
	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
}

static xcb_atom_t
get_atom(const char* name)
{
	xcb_atom_t atom;
	xcb_generic_error_t* error;
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t* reply;

	cookie = xcb_intern_atom(c, 0, strlen(name), name);
	reply = xcb_intern_atom_reply(c, cookie, &error);

	if (error != NULL) {
		fprintf(stderr, "Error: xcb_intern_atom(): %d\n", (int)(error->error_code));
	}

	atom = reply->atom;
	free(reply);
	return atom;
}

static void
create_window(void)
{
	c = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(c)) {
		fprintf(stderr, "Error: xcb_connect()\n");
	}
	
	scrn = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	if (scrn == NULL) {
		fprintf(stderr, "Error: xcb_screen_roots_iterator()\n");
	}

	width = height = 600;
	pc = (width * height);

	px = malloc(sizeof(uint32_t) * pc);
	if (px == NULL) {
		fprintf(stderr, "Error: malloc()\n");
	}

	window = xcb_generate_id(c);
	gc = xcb_generate_id(c);

	xcb_create_window_aux(c, XCB_COPY_FROM_PARENT, window, scrn->root, 0,
			      0, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			      scrn->root_visual, XCB_CW_EVENT_MASK,
			      (const xcb_create_window_value_list_t []) {{
					      .event_mask = XCB_EVENT_MASK_EXPOSURE |
						      XCB_EVENT_MASK_KEY_PRESS |
						      XCB_EVENT_MASK_STRUCTURE_NOTIFY
				      }}
		);

	xcb_create_gc(c, gc, window, 0, NULL);

	image = xcb_image_create_native(c, width, height, XCB_IMAGE_FORMAT_Z_PIXMAP,
					scrn->root_depth, px, (sizeof(uint32_t) * pc),
					(uint8_t *)(px));

	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, get_atom("_NET_WM_NAME"),
			    get_atom("UTF8_STRING"), 8, strlen("xcbsimple"), "xcbsimple");

	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_CLASS,
			    XCB_ATOM_STRING, 8, strlen("xcbsimple\0xcbsimple\0"),
			    "xcbsimple\0xcbsimple\0");

	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, get_atom("WM_PROTOCOLS"),
			    XCB_ATOM_ATOM, 32, 1, (const xcb_atom_t []) { get_atom("WM_DELETE_WINDOW") });

	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, get_atom("_NET_WM_STATE"),
			    XCB_ATOM_ATOM, 32, 1, (const xcb_atom_t []) { get_atom("_NET_WM_STATE_FULLSCREEN") });
	
	xcb_map_window(c, window);
	xcb_flush(c);
}

static void
delete_window(void)
{
	xcb_free_gc(c, gc);
	xcb_image_destroy(image);
	xcb_disconnect(c);
}

int
main(void)
{
	create_window();

	ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	fz_register_document_handlers(ctx);
	doc = fz_open_document(ctx, "auug97.pdf");

	page = fz_load_page(ctx, doc, 1);
	page_count = fz_count_pages(ctx, doc);
	printf("Page count: %d\n", page_count);

	ctm = fz_scale(1, 1);
	ctm = fz_pre_rotate(ctm, 0);

	for (int i = 0; i < page_count; i++) {
		char str[50];
		snprintf(str, sizeof(str), "out/out%d.png", i);
		pix = fz_new_pixmap_from_page_number(ctx, doc, i, ctm, fz_device_rgb(ctx), 0);
		fz_save_pixmap_as_png(ctx, pix, str);
	}

	fz_cleanup(ctx, page, doc, pix);
	delete_window();
	
	return 0;
}

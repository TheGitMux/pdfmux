#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_icccm.h>

#define WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME "WM_PROTOCOLS"
#define WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME_LENGTH 12

#define WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME "WM_DELETE_WINDOW"
#define WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME_LENGTH 16

static xcb_gc_t gc_font_get (xcb_connection_t *c,
                             xcb_screen_t     *screen,
                             xcb_window_t      window,
                             const char       *font_name);

static void text_draw (xcb_connection_t *c,
                       xcb_screen_t     *screen,
                       xcb_window_t      window,
                       int16_t           x1,
                       int16_t           y1,
                       const char       *label);

static void
process_event(xcb_generic_event_t *generic_event, bool *is_running, xcb_window_t window,
	      xcb_atom_t window_manager_window_delete_protocol)
{
	switch(XCB_EVENT_RESPONSE_TYPE(generic_event)) {
        case XCB_CLIENT_MESSAGE: {
		xcb_client_message_event_t *client_message_event = (xcb_client_message_event_t*) generic_event;
		if(client_message_event->window == window) {
			if(client_message_event->data.data32[0] == window_manager_window_delete_protocol) {
				uint32_t timestamp_milliseconds = client_message_event->data.data32[1];
				printf("Closed at timestamp %i ms\n", timestamp_milliseconds);
				*is_running = false;
			}
		}
        } break;
        case XCB_EXPOSE: {
		xcb_expose_event_t *expose_event = (xcb_expose_event_t*) generic_event;
		if(expose_event->window == window) {
			printf("EXPOSE: Rect(%i, %i, %i, %i)\n", expose_event->x, expose_event->y, expose_event->width, expose_event->height);
		}
         } break;
	}
}

static void
text_draw(xcb_connection_t *c, xcb_screen_t *s, xcb_window_t w,
	  int16_t x1, int16_t y1, const char *label)
{
	xcb_void_cookie_t	cookie_gc;
	xcb_void_cookie_t	cookie_text;
	xcb_generic_error_t	*error;
	xcb_gcontext_t		gc;
	uint8_t			length;

	length = strlen(label);

	gc = gc_font_get(c, s, w, "7x13");

	cookie_text = xcb_image_text_8_checked(c, length, w, gc,
					       x1, y1, label);
	error = xcb_request_check(c, cookie_text);
	if (error) {
		fprintf(stderr, "ERROR: can't paste text: %d\n", error->error_code);
		xcb_disconnect(c);
		exit(-1);
	}

	cookie_gc = xcb_free_gc(c, gc);
	error = xcb_request_check(c, cookie_gc);
	if (error) {
		fprintf(stderr, "ERROR: cannot free gc: %d\n", error->error_code);
		xcb_disconnect(c);
		exit(-1);
	}
}

static xcb_gc_t
gc_font_get(xcb_connection_t *c, xcb_screen_t *s,
	    xcb_window_t w, const char *fname)
{
	uint32_t		value_list[3];
	xcb_void_cookie_t	cookie_font;
	xcb_void_cookie_t	cookie_gc;
	xcb_generic_error_t	*error;
	xcb_font_t		font;
	xcb_gcontext_t		gc;
	uint32_t		mask;

	font = xcb_generate_id(c);
	cookie_font = xcb_open_font_checked(c, font, strlen(fname), fname);

	error = xcb_request_check(c, cookie_font);
	if (error) {
		fprintf(stderr, "ERROR: Cannot open font: %d\n", error->error_code);
		xcb_disconnect(c);
		return -1;
	}

	gc = xcb_generate_id(c);
	mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
	value_list[0] = s->black_pixel;
	value_list[1] = s->white_pixel;
	value_list[2] = font;
	cookie_gc = xcb_create_gc_checked(c, gc, w, mask, value_list);
	error = xcb_request_check(c, cookie_gc);
	if (error) {
		fprintf(stderr, "ERROR: cannot open font: %d\n", error->error_code);
		xcb_disconnect(c);
		exit(-1);
	}
    
	cookie_font = xcb_close_font_checked(c, font);
	error = xcb_request_check(c, cookie_font);
	if (error) {
		fprintf(stderr, "ERROR: cannot close font: %d\n", error->error_code);
		xcb_disconnect(c);
		exit(-1);
	}

	return gc;
}

int
main()
{
	const char *window_title = "pdfmux";
	int16_t window_x = 0;
	int16_t window_y = 0;
	uint16_t window_width = 720;
	uint16_t window_height = 480;
	uint32_t window_background_color[] = {0, 0, 0, 255};
	bool is_window_resizable = true;
	bool is_wait_event = true;

	int screen_number;
	xcb_connection_t *connection = xcb_connect(NULL, &screen_number);
	assert(!xcb_connection_has_error(connection));

	xcb_intern_atom_cookie_t intern_atom_cookie;
	xcb_intern_atom_reply_t *intern_atom_reply;

	intern_atom_cookie = xcb_intern_atom(connection, true, WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME_LENGTH,
					     WINDOW_MANAGER_PROTOCOLS_PROPERTY_NAME);
	intern_atom_reply = xcb_intern_atom_reply(connection, intern_atom_cookie, NULL);
	xcb_atom_t window_manager_protocols_property = intern_atom_reply->atom;
	free(intern_atom_reply);

	intern_atom_cookie = xcb_intern_atom(connection, true, WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME_LENGTH,
					     WINDOW_MANAGER_DELETE_WINDOW_PROTOCOL_NAME);
	intern_atom_reply = xcb_intern_atom_reply(connection, intern_atom_cookie, NULL);
	xcb_atom_t window_manager_window_delete_protocol = intern_atom_reply->atom;
	free(intern_atom_reply);

	xcb_screen_t *screen = xcb_aux_get_screen(connection, screen_number);

	xcb_window_t window = xcb_generate_id(connection);

	xcb_create_window_aux(connection,
			      screen->root_depth,
			      window,
			      screen->root,
			      window_x, window_y, window_width, window_height,
			      0,
			      XCB_WINDOW_CLASS_INPUT_OUTPUT,
			      screen->root_visual,
			      XCB_CW_EVENT_MASK | XCB_CW_BACK_PIXEL, &(xcb_create_window_value_list_t) {
				      .background_pixel = window_background_color[2] | (window_background_color[1] << 8) |
				      (window_background_color[0] << 16) | (window_background_color[3] << 24),
				      .event_mask = XCB_EVENT_MASK_EXPOSURE,
			      });

	xcb_icccm_set_wm_name(connection, window, XCB_ATOM_STRING, 8, strlen(window_title), window_title);
	xcb_icccm_set_wm_protocols(connection, window, window_manager_protocols_property, 1,
				   &(xcb_atom_t) {window_manager_window_delete_protocol});

	if (!is_window_resizable) {
		xcb_size_hints_t window_size_hints;
		xcb_icccm_size_hints_set_min_size(&window_size_hints, window_width, window_height);
		xcb_icccm_size_hints_set_max_size(&window_size_hints, window_width, window_height);
		xcb_icccm_set_wm_size_hints(connection, window, XCB_ATOM_WM_NORMAL_HINTS, &window_size_hints);
	}

	xcb_gcontext_t gc;
	xcb_drawable_t win;
	uint32_t mask;
	uint32_t value[1];
	
	gc = xcb_generate_id(connection);
	mask = XCB_GC_BACKGROUND;
	value[0] = screen->black_pixel;
	
	xcb_create_gc(connection, gc, win, mask, value);
	
	xcb_map_window(connection, window);
	assert(xcb_flush(connection));

	bool is_running = true;

	xcb_generic_event_t *generic_event;

	while (is_running) {
		if (is_wait_event) {
			generic_event = xcb_wait_for_event(connection);
			assert(generic_event);
			process_event(generic_event, &is_running, window, window_manager_window_delete_protocol);
			char *text = "Press ESC to exit...";
			text_draw(connection, screen, window, 10, 300, text);
		} else {
			while ((generic_event = xcb_poll_for_event(connection))) {
				process_event(generic_event, &is_running, window, window_manager_window_delete_protocol);
			}
		}

		printf("UPDATE!\n");
	}

	xcb_disconnect(connection);
	return EXIT_SUCCESS;
}

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <png.h>

#include <xcb/xcb.h>
#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_XCB_KHR
#define MAX_NUM_IMAGES 5

struct vkcube_buffer {
	struct gbm_bo *gbm_bo;
	VkDeviceMemory mem;
	VkImage image;
	VkImageView view;
	VkFramebuffer framebuffer;
	VkFence fence;
	VkCommandBuffer cmd_buffer;

	uint32_t fb;
	uint32_t stride;
};

struct pdf {
	xcb_connection_t	*conn;
	xcb_window_t		window;
	xcb_atom_t		atom_wm_protocols;
	xcb_atom_t		atom_wm_delete_window;

	bool			protected;

	struct {
		VkDisplayModeKHR display_mode;
	} khr;

	VkSwapchainKHR		swap_chain;

	drmModeCrtc		*crtc;
	drmModeConnector	*connector;
	uint32_t width, height;

	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceMemoryProperties	memory_properties;
	VkDevice device;
	VkRenderPass render_pass;
	VkQueue queue;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkDeviceMemory mem;
	VkBuffer buffer;
	VkDescriptorSet descriptor_set;
	VkSemaphore sempahore;
	VkCommandPool cmd_pool;

	void *map;
	uint32_t vertex_offset, colors_offset, normals_offset;

	struct timeval start_tv;
	VkSurfaceKHR surface;
	VkFormat image_format;
	struct vkcube_buffer buffers[MAX_NUM_IMAGES];
	uint32_t image_count;
	int current;
};

static xcb_atom_t
get_atom(struct xcb_connection_t *conn, const char *name)
{
	xcb_intern_atom_cookie_t	cookie;
	xcb_intern_atom_reply_t		*reply;
	xcb_atom_t			atom;

	cookie = xcb_intern_atom(conn, 0, strlen(name), name);
	reply = xcb_intern_atom_reply(conn, cookie, NULL);
	if (reply) {
		atom = reply->atom;
	} else {
		atom = XCB_NONE;
	}

	free(reply);
	return atom;
}

int
main()
{
	struct pdf *pdf;

	pdf = (struct pdf*) malloc(sizeof(struct pdf));
	
	static const char title[] = "pdfmux";
	pdf->conn = xcb_connect(0, 0);

	if (xcb_connection_has_error(pdf->conn)) {
		return -1;
	}

	pdf->window = xcb_generate_id(pdf->conn);

	uint32_t window_values[] = {
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_KEY_PRESS
	};

	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(pdf->conn));

	xcb_create_window(pdf->conn, XCB_COPY_FROM_PARENT, pdf->window, iter.data->root,
			  0, 0, 1024, 768, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  iter.data->root_visual, XCB_CW_EVENT_MASK, window_values);

	pdf->atom_wm_protocols = get_atom(pdf->conn, "WM_PROTOCOLS");
	pdf->atom_wm_delete_window = get_atom(pdf->conn, "WM_DELETE_WINDOW");
	
	xcb_change_property(pdf->conn, XCB_PROP_MODE_REPLACE, pdf->window, pdf->atom_wm_protocols,
			    XCB_ATOM_ATOM, 32, 1, &pdf->atom_wm_delete_window);

	xcb_change_property(pdf->conn, XCB_PROP_MODE_REPLACE, pdf->window, get_atom(pdf->conn, "_NET_WM_NAME"),
			    get_atom(pdf->conn, "UTF8_STRING"), 8, strlen(title), title);

	xcb_map_window(pdf->conn, pdf->window);

	xcb_flush(pdf->conn);
	
	return 0;

}

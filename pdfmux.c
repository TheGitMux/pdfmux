#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mupdf/fitz.h>
#include <vulkan/vulkan.h>

int
main(void)
{
	int page_count, page_number = 0;
	fz_pixmap* pix;
	
	fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);

	fz_try(ctx) {
		fz_register_document_handlers(ctx);
	}
	fz_catch(ctx) {
		fprintf(stderr, "Cannot register document handlers: %s\n", fz_caught_message(ctx));
		fz_drop_context(ctx);
		return EXIT_FAILURE;
	}

	fz_document* doc = fz_open_document(ctx, "auug97.pdf");

	fz_page* page = fz_load_page(ctx, doc, 1);
	page_count = fz_count_pages(ctx, doc);
	printf("Page count: %d\n", page_count);

	fz_matrix ctm = fz_scale(1, 1);
	ctm = fz_pre_rotate(ctm, 0);

	for (int i = 0; i < page_count; i++) {
		char str[50];
		snprintf(str, sizeof(str), "out/out%d.png", i);
		pix = fz_new_pixmap_from_page_number(ctx, doc, i, ctm, fz_device_rgb(ctx), 0);
		fz_save_pixmap_as_png(ctx, pix, str);
	}
	
	fz_drop_pixmap(ctx, pix);
	fz_drop_page(ctx, page);
	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);

	return 0;
}

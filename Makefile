CC = gcc

BASE_C_GLOBS = $(wildcard *.c)
BASE_OBJ_GLOBS = $(BASE_C_GLOBS:.c=.o)

CFLAGS = -Os -g -Werror
LDFLAGS = -ldjvulibre -lmupdf -lSDL -lm -ljpeg -lfreetype -lz -lharfbuzz -lmupdf-third -ljbig2dec -lgumbo -lopenjp2 -lxcb -lvulkan -lxcb -lxcb-image

CC_CMD = $(CC) -c -o $@ $< $(CFLAGS)
LD_CMD = $(CC) -o $@ $< $(LDFLAGS)

PROG = pdfmux

all: $(PROG)

$(PROG): $(BASE_OBJ_GLOBS)
	$(LD_CMD)

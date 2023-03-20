CC = gcc

BASE_C_GLOBS = $(wildcard *.c)
BASE_OBJ_GLOBS = $(BASE_C_GLOBS:.c=.o)

CFLAGS = -Os -g -Werror
LDFLAGS = `pkg-config --libs pdfio xcb libpng xcb-cursor xcb-image xcb-keysyms`

CC_CMD = $(CC) -c -o $@ $< $(CFLAGS)
LD_CMD = $(CC) -o $@ $< $(LDFLAGS)

PROG = pdfmux

all: $(PROG)

$(PROG): $(BASE_OBJ_GLOBS)
	$(LD_CMD)

CC = gcc
CFLAGS = `pkg-config --cflags gtk4`
LIBS = `pkg-config --libs gtk4`

all: dark

style_embedded.h: style.css
	xxd -i style.css  > resources/style_embedded.h


dark: dark.c  style_embedded.h
	$(CC) $(CFLAGS) -o dark dark.c $(LIBS)

clean:
	rm -f dark  compile_commands.json

.PHONY: all clean
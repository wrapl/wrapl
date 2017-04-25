.PHONY: clean all

all: ../../bin/rabs

sources = \
	cache.c \
	context.c \
	rabs.c \
	target.c \
	util.c \
	vfs.c \
	builtins.s

CFLAGS = `pkg-config --cflags lua53` -I. -g -O3
LDFLAGS = `pkg-config --libs lua53` -lm -lgc -lsqlite3 -lnettle -lHX -g

../../bin/rabs: $(sources) *.h builtins.lua
	gcc $(CFLAGS) $(sources) $(LDFLAGS) -o $@

clean:
	rm ../../bin/rabs

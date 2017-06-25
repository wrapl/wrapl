.PHONY: clean all

all: ../../bin/rabs

sources = \
	cache.c \
	context.c \
	rabs.c \
	target.c \
	util.c \
	vfs.c \
	builtins.s \
	lfs.c

CFLAGS = -I. -g -O3
LDFLAGS = -llua -lm -lgc -lsqlite3 -lnettle -lHX -g

../../bin/rabs: $(sources) *.h builtins.lua
	gcc $(CFLAGS) $(sources) $(LDFLAGS) -o $@

clean:
	rm ../../bin/rabs

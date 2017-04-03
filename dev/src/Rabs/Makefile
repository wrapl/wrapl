.PHONY: clean all

all: ../../bin/rabs

sources = \
	cache.c \
	context.c \
	map.c \
	rabs.c \
	target.c \
	util.c \
	vfs.c \
	builtins.s

CFLAGS = -I. -g
LDFLAGS = -llua -lm -lgc -lsqlite3 -lnettle -lHX -g

../../bin/rabs: $(sources) *.h builtins.lua
	gcc $(CFLAGS) $(sources) $(LDFLAGS) -o $@

clean:
	rm $(objects)
	rm rabs

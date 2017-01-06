.PHONY: clean all

all: rabs

objects = \
	cache.o \
	context.o \
	map.o \
	rabs.o \
	target.o \
	util.o \
	vfs.o

CFLAGS = -I. -g
LDFLAGS = -llua -lm -lgc -lsqlite3 -lnettle -g

$(objects): %.o: %.c *.h
	gcc -c $(CFLAGS) $< -o $@

rabs: $(objects)
	gcc $(objects) $(LDFLAGS) -o $@

clean:
	rm $(objects)
	rm rabs

CC = cc
# Native feature visibility keeps BSD special modes and device macros exposed.
CPPFLAGS = -D_DEFAULT_SOURCE -D_NETBSD_SOURCE -D_DARWIN_C_SOURCE -D_FILE_OFFSET_BITS=64 -Iinclude
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS =
LDLIBS =

PROGRAM = myls
SOURCES = src/main.c src/options.c src/entry.c src/listing.c src/format.c src/util.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = include/options.h include/entry.h include/listing.h include/format.h include/util.h

.PHONY: all clean
all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(OBJECTS): $(HEADERS) Makefile

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(PROGRAM) myls-sanitize

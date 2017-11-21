CC=gcc
CFLAGS=-c -Wall -Wextra -pedantic -Werror
LDFLAGS=
SOURCES=bag.c ringbuf.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=bag

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

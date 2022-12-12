OBJECTS = h264grabber.o
OPTS = -march=armv5te -mcpu=arm926ej-s
LIB = -lpthread

all: h264grabber

h264grabber.o: h264grabber.c $(HEADERS)
	$(CC) -c $< $(OPTS) -fPIC -O2 -Wall -o $@

h264grabber: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIB) $(OPTS) -fPIC -O2 -Wall -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f h264grabber
	rm -f $(OBJECTS)

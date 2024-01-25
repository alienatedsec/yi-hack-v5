OBJECTS_C = ipc_cmd.o ptz.o
OBJECTS_R = ipc_read.o
OBJECTS_N = ipc_notify.o
OBJECTS_2 = ipc2file.o
LIBS = -lpthread -lrt -lm
LIBS_2 = -lrt -lpthread

all: ipc_cmd ipc_read ipc_notify ipc2file ipc_multiplex.so

%.o: %.c $(HEADERS)
	$(CC) -c $< -D_GNU_SOURCE -std=c99 -fPIC -O2 -o $@

ipc_multiplex.so: ipc_multiplex.c $(HEADERS)
	$(CC) $< -D_GNU_SOURCE -std=c99 -fPIC -O2 -shared -o $@

ipc_cmd: $(OBJECTS_C)
	$(CC) $(OBJECTS_C) $(LIBS) -fPIC -O2 -o $@
	$(STRIP) $@

ipc_read: $(OBJECTS_R)
	$(CC) $(OBJECTS_R) $(LIBS) -fPIC -O2 -o $@
	$(STRIP) $@

ipc_notify: $(OBJECTS_N)
	$(CC) $(OBJECTS_N) $(LIBS) -fPIC -O2 -o $@
	$(STRIP) $@

ipc2file: $(OBJECTS_2)
	$(CC) $(OBJECTS_2) $(LIBS_2) -fPIC -Os -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f ipc_cmd
	rm -f ipc_read
	rm -f ipc_notify
	rm -f ipc2file
	rm -f ipc_multiplex.so
	rm -f $(OBJECTS_C)
	rm -f $(OBJECTS_R)
	rm -f $(OBJECTS_N)
	rm -f $(OBJECTS_2)

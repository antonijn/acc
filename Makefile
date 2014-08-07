
CFLAGS += -Iinclude -std=c89 -pedantic-errors

release:
	$(CC) $(CFLAGS) -O2 -DNDEBUG -o acc src/*.c

debug:
	$(CC) $(CFLAGS) -o acc src/*.c


CFLAGS += -Iinclude -std=c89

release:
	$(CC) $(CFLAGS) -O2 -o acc src/*.c

debug:
	$(CC) $(CFLAGS) -o acc src/*.c

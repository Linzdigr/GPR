CC=gcc
CFLAGS=-I.
OBJ = main.o mcp4725.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gauge: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~

CC=g++
CFLAGS=-I. -lpthread
OBJ = main.o GPR.o MCP4921.o waveforms.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nalláma: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~

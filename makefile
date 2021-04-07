CC=g++
CFLAGS=-I. -lpthread -lasound
OBJ = main.o GPR.o MCP4921/MCP4921.o waveforms.o audio.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nalláma: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~

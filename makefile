CC=g++
CFLAGS=-I. -lpthread -lasound -lfftw3 -lm
OBJ = main.o GPR.o MCP4921/MCP4921.o waveforms.o recorder.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nalláma: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~

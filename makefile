CC=g++
CFLAGS=-I. -O3
OBJ = main.o GPR.o mcp4921.o waveeform.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gpr: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~

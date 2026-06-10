CC=gcc
CFLAGS = -Wall -ggdb

ODIR=obj

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
DEPS = $(SRC:.c=.h)

LDFLAGS =
LIBS = -lm

%.o: %.c $(DEPS)
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)

insee: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS) -O3

.PHONY: clean

clean:
	rm -f *.o insee

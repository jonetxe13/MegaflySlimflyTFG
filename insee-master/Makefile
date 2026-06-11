CC=gcc
CFLAGS = -Wall -ggdb 
#-O0 -fno-omit-frame-pointer -fsanitize=address,undefined
LDFLAGS = 
#-fsanitize=address,undefined 

ODIR=obj

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
DEPS = $(SRC:.c=.h)

#LDFLAGS =
LIBS = -lm

%.o: %.c $(DEPS)
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)

insee: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o insee

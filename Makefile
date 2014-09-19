CFLAGS = -Wall -Wshadow -ansi -pedantic -O2
LDFLAGS = -lalpm

SRC = pacfree.c
OBJ = ${SRC:.c=.o}

all: pacfree

pacfree: ${OBJ} 
	gcc -o $@ $^ $(LDFLAGS)	
	
%.o:%.c
	gcc -c $(CFLAGS) $<

clean:
	rm -f *.o

.PHONY: all clean

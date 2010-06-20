CC      = gcc
DIET    = diet -Os
LDFLAGS = -s
LIBS	  = -lowfat
CFLAGS  = -Os

all: do-after

do-after: do-after.o
	$(DIET) $(CC) $(LDFLAGS) do-after.o -o do-after $(LIBS)

do-after.o: do-after.c
	$(CC) $(CFLAGS) -c do-after.c

test: do-after
	./do-after 3 echo 'test 1'
	./do-after -v 3 echo 'test 2'
	./do-after -v -v 3 echo 'test 3'

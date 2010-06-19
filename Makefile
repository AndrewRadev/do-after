CC      = gcc
DIET    = diet
LDFLAGS = -s -lowfat
CFLAGS  = -O2

all: do-after

do-after: do-after.o
	$(DIET) $(CC) -o do-after $(LDFLAGS) do-after.o

do-after.o: do-after.c
	$(CC) $(CFLAGS) -c do-after.c

test: do-after
	./do-after 3 echo 'test 1'
	./do-after -v 3 echo 'test 2'
	./do-after -v -v 3 echo 'test 3'

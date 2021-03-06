CC      = gcc
DIET    = diet -Os
LDFLAGS = -s
LIBS    = -lowfat
CFLAGS  = -Os
#CFLAGS  = -Os -DDEBUG -DLOG

all: do-after

do-after: do-after.o
	$(DIET) $(CC) $(LDFLAGS) do-after.o -o do-after $(LIBS)

do-after.o: do-after.c
	$(DIET) $(CC) $(CFLAGS) -c do-after.c

test: do-after
	./do-after 3 echo 'test 1'
	./do-after -v 3 echo 'test 2'
	./do-after -v -v 3 echo 'test 3'
	./do-after -vv 3 echo 'test 4'

tgz:
	tar cvf do-after.tar do-after.c Makefile README README.Nikola_Vladov
	gzip do-after.tar

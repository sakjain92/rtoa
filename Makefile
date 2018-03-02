CFLAGS=-g3 -Wall -Wextra -Wconversion -pedantic -Wshadow -Wpointer-arith -Wcast-qual \
        -Wstrict-prototypes -Wmissing-prototypes
CC=gcc
LDFLAGS=-lm

all: exp

opt_rms:
	$(CC) $(CFLAGS) -DDEBUG opt_rms.c common.c -o opt_rms.elf $(LDFLAGS)

pt_rms:
	$(CC) $(CFLAGS) -DDEBUG pt_rms.c common.c -o pt_rms.elf $(LDFLAGS)

capa:
	$(CC) $(CFLAGS) -DDEBUG capa.c common.c -o capa.elf $(LDFLAGS)

zsrms:
	$(CC) $(CFLAGS) -DDEBUG zsrms.c common.c -o zsrms.elf $(LDFLAGS)

#vopt:
#	gcc -g3 -Wall -Wextra -c pt_rms.c -o obj/pt_rms.o -lm
#	gcc -g3 -Wall -DDEBUG -Wextra obj/pt_rms.o vopt_rms.c -o vopt_rms.elf -lm

rms:
	$(CC) $(CFLAGS) -DDEBUG rms.c common.c -o rms.elf $(LDFLAGS)

exp:
	$(CC) $(CFLAGS) pt_rms.c opt_rms.c capa.c zsrms.c rms.c main.c common.c -o exp.elf $(LDFLAGS)

clean:
	rm -f *.elf obj/*.o

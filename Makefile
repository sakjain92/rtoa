all: exp

opt_rms:
	gcc -g3 -Wall -DDEBUG -Wextra opt_rms.c common.c -o opt_rms.elf -lm

pt_rms:
	gcc -g3 -Wall -DDEBUG -Wextra pt_rms.c common.c -o pt_rms.elf -lm

capa:
	gcc -g3 -Wall -DDEBUG -Wextra capa.c common.c -o capa.elf -lm

zsrms:
	gcc -g3 -Wall -DDEBUG -Wextra zsrms.c common.c -o zsrms.elf -lm

vopt:
	gcc -g3 -Wall -Wextra -c opt_rms.c -o obj/opt_rms.o -lm
	gcc -g3 -Wall -DDEBUG -Wextra obj/opt_rms.o vopt_rms.c -o vopt_rms.elf -lm

rms:
	gcc -g3 -Wall -DDEBUG -Wextra rms.c common.c -o rms.elf -lm

exp:
	gcc -g3 -Wall -Wextra pt_rms.c opt_rms.c capa.c zsrms.c rms.c main.c common.c -o exp.elf -lm
clean:
	rm -f *.elf obj/*.o

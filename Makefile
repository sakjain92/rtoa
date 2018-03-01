all: exp

opt_rms:
	gcc -g3 -Wall -DDEBUG -Wextra opt_rms.c common.c -o opt_rms.elf -lm

pt_rms:
	gcc -g3 -Wall -DDEBUG -Wextra pt_rms.c common.c -o pt_rms.elf -lm

capa:
	gcc -g3 -Wall -DDEBUG -Wextra capa.c common.c -o capa.elf -lm

zsrms:
	gcc -g3 -Wall -DDEBUG -Wextra zsrms.c common.c -o zsrms.elf -lm

exp:
	gcc -g3 -Wall -Wextra pt_rms.c opt_rms.c capa.c zsrms.c main.c common.c -o exp.elf -lm
clean:
	rm -f *.elf

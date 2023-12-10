all: emu 

emu: emu.o
	gcc emu.o -o emu

emu.o: emu.c
	gcc -c -ansi -pedantic-errors -Wall *.c -std=gnu99 emu.c

clean:
	rm -f emu *.o

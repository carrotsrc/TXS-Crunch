pcmr.o:
	gcc -ggdb -c pcmr.c -o obj/pcmr.o

entry.o:
	gcc -ggdb -c entry.c -o obj/entry.o

full: pcmr.o entry.o
	gcc -lasound -lpthread -o crunch obj/pcmr.o obj/entry.o



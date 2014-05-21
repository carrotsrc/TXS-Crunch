pcmrw.o:
	gcc -ggdb -c pcmrw.c -o obj/pcmrw.o

pcmrw: pcmrw.o
	gcc -lasound -lpthread -o crunch obj/pcmrw.o



stream.o:
	gcc -ggdb -c stream.c -o obj/stream.o

bufproc.o: stream.o
	gcc -ggdb -c bufproc.c -o obj/bufproc.o

entry.o: stream.o bufproc.o
	gcc -ggdb -c entry.c -o obj/entry.o

full: stream.o bufproc.o entry.o
	gcc -lasound -lpthread -o crunch obj/stream.o obj/bufproc.o obj/entry.o



all: lib

lib: mythread.o
	ar rcs mythread.a mythread.o

mythread.o: mythread.c
	gcc -c mythread.c

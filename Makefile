main.o: main.c
	gcc -c main.c -o main.o

main: main.o
	gcc main.o -o main -pthread

all: main

clean:
	rm *.o main

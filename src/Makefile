
CC = g++
CFLAGS = -Wall -pedantic -std=c++2a -fsanitize=address -g -O1
LDFLAGS = -lasan

INCLUDES = -I../include

all: main.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.o -o out

main.o: main.cpp
	$(CC) $(CFLAGS) $(INCLUDES) main.cpp -c -o main.o

clean:
	rm -f *.o
	rm -f out

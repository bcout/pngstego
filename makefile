CC := gcc
CFLAGS := -Wall -g

pngstego: pngstego.o
	gcc -Wall -g -o pngstego pngstego.o -lpng -lm

pngstego.o: pngstego.c
	gcc -Wall -g -c -o pngstego.o pngstego.c -lpng -lm

clean:
	rm -f *.o pngstego

CC := gcc
CFLAGS := -Wall -g

pngstego: pngstego.o
	gcc -Wall -g -o pngstego pngstego.o -lpng

pngstego.o: pngstego.c
	gcc -Wall -g -c -o pngstego.o pngstego.c -lpng

clean:
	rm -f *.o pngstego

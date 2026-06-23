flags = -c
sources = $(wildcard *.c)
headers = $(wildcard *.h)
objects = $(sources:.c=.o$)
exec = main.out

main:
	gcc $(flags) $(sources) $(headers)
	gcc $(objects) -o $(exec)

clean:
	rm *.o
	rm *.h.gch
	rm $(exec)


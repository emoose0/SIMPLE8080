flags = -c
sources = $(wildcard *.c)
headers = $(wildcard *.h)
objects = $(sources:.c=.o$)
exec = s8080

main:
	gcc $(flags) $(sources) $(headers)
	gcc $(objects) -o $(exec)

clean:
	rm *.o
	rm *.h.gch
	rm $(exec)


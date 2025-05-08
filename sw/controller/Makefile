CFLAGS = -Wall

OBJECTS = main.o usbjoypad.o

TARFILES = Makefile main.c \
	usbjoypad.h usbjoypad.c

main : $(OBJECTS)
	cc $(CFLAGS) -o main $(OBJECTS) -lusb-1.0 -pthread

main.tar.gz : $(TARFILES)
	rm -rf main
	mkdir main
	ln $(TARFILES) main
	tar zcf lab2.tar.gz main
	rm -rf lab2

main.o : main.c usbjoypad.h
usbjoypad.o : usbjoypad.c usbjoypad.h

.PHONY : clean
clean :
	rm -rf *.o main

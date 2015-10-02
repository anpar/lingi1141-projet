CC=gcc
CFLAGS=-Wall -Werror -Wextra -Wshadow -g

all: test 

packet-o: src/packet_implem.c src/packet_interface.h
	$(CC) -c -o src/packet_implem.o src/packet_implem.c $(CFLAGS)

test-o: tests/packet_test.c src/packet_implem.c src/packet_interface.h
	$(CC) -c -o tests/packet_test.o tests/packet_test.c $(CFLAGS)

test: packet-o test-o tests/packet_test.o src/packet_implem.o
	$(CC) -o tests/test tests/packet_test.o src/packet_implem.o -lcunit
	tests/test $(CFLAGS)

clean: 
	rm -r src/*.o tests/*.o

mrproper: clean
	rm tests/test



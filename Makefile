all: test 

packet-o: src/packet_implem.c src/packet_interface.h
	gcc -c -o src/packet_implem.o src/packet_implem.c

test-o: tests/packet_test.c src/packet_implem.c src/packet_interface.h
	gcc -c -o tests/packet_test.o tests/packet_test.c

test: packet-o test-o tests/packet_test.o src/packet_implem.o
	gcc -o tests/test tests/packet_test.o src/packet_implem.o -lcunit
	tests/test

clean: 
	rm -r src/*.o tests/*.o

mrproper: clean
	rm tests/test



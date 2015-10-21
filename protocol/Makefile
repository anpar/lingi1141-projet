CFLAGS=-Wall -Werror -Wshadow -Wextra -g

all: receiver

receiver: receiver.o read_loop.o real_address.o wait_for_sender.o create_socket.o packet_implem.o
	gcc $(CFLAGS) -o receiver receiver.o read_loop.o real_address.o wait_for_sender.o create_socket.o packet_implem.o -lz

receiver.o: src/receiver.c src/read_loop.h src/real_address.h src/wait_for_sender.h src/create_socket.h src/packet_interface.h
	gcc $(CFLAGS) -c src/receiver.c

read_loop.o: src/read_loop.c src/read_loop.h
	gcc $(CFLAGS) -c src/read_loop.c

real_address.o: src/real_address.c src/real_address.h
	gcc $(CFLAGS) -c src/real_address.c

wait_for_sender.o: src/wait_for_sender.c src/wait_for_sender.h
	gcc $(CFLAGS) -c src/wait_for_sender.c

create_socket.o: src/create_socket.c src/create_socket.h
	gcc $(CFLAGS) -c src/create_socket.c

packet_implem.o: src/packet_implem.c src/packet_interface.h
	gcc $(CFLAGS) -c src/packet_implem.c -lz 

tests:  tests.o read_loop.o packet_implem.o
	gcc $(CFLAGS) -o test tests.o read_loop.o packet_implem.o -lz -lcunit

tests.o: tests/tests.c src/read_loop.h 
	gcc $(CFLAGS) -c tests/tests.c

clean:
	rm -rf receiver sender *.o *.c~ *.h~


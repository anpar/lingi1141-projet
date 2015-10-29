CFLAGS=-Wall -Werror -Wshadow -Wextra -g

all: receiver sender

sender: sender.o sender_core.o real_address.o wait_for_sender.o create_socket.o packet_implem.o
	gcc $(CFLAGS) -o sender sender.o sender_core.o real_address.o wait_for_sender.o create_socket.o packet_implem.o -lz -lrt

sender.o: src/sender.c src/sender_core.h src/real_address.h src/wait_for_sender.h src/create_socket.h src/packet_interface.h
	gcc $(CFLAGS) -c src/sender.c

sender_core.o: src/sender_core.c src/sender_core.h
	gcc $(CFLAGS) -c src/sender_core.c

receiver: receiver.o receiver_core.o real_address.o wait_for_sender.o create_socket.o packet_implem.o
	gcc $(CFLAGS) -o receiver receiver.o receiver_core.o real_address.o wait_for_sender.o create_socket.o packet_implem.o -lz

receiver.o: src/receiver.c src/receiver_core.h src/real_address.h src/wait_for_sender.h src/create_socket.h src/packet_interface.h
	gcc $(CFLAGS) -c src/receiver.c

receiver_core.o: src/receiver_core.c src/receiver_core.h
	gcc $(CFLAGS) -c src/receiver_core.c

real_address.o: src/real_address.c src/real_address.h
	gcc $(CFLAGS) -c src/real_address.c

wait_for_sender.o: src/wait_for_sender.c src/wait_for_sender.h
	gcc $(CFLAGS) -c src/wait_for_sender.c

create_socket.o: src/create_socket.c src/create_socket.h
	gcc $(CFLAGS) -c src/create_socket.c

packet_implem.o: src/packet_implem.c src/packet_interface.h
	gcc $(CFLAGS) -c src/packet_implem.c -lz

tests:  tests.o receiver_core.o packet_implem.o real_address.o create_socket.o wait_for_sender.o sender_core.o
	gcc $(CFLAGS) -o test tests.o receiver_core.o sender_core.o packet_implem.o real_address.o create_socket.o wait_for_sender.o -lz -lcunit -pthread -lrt
	./test

tests.o: tests/tests.c src/receiver_core.h src/sender_core.h
	gcc $(CFLAGS) -c tests/tests.c

clean:
	rm -rf receiver sender test *.o *.c~ *.h~ out/*

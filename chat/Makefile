CFLAGS=-Wall -Werror -Wshadow -Wextra -g

all: chat

chat: chat.o read_write_loop.o create_socket.o real_address.o wait_for_client.o
	gcc $(CFLAGS) -o chat chat.o read_write_loop.o create_socket.o real_address.o wait_for_client.o

chat.o: chat.c read_write_loop.h create_socket.h real_address.h wait_for_client.h
	gcc $(CFLAGS) -c chat.c

read_write_loop.o: read_write_loop.c read_write_loop.h
	gcc $(CFLAGS) -c read_write_loop.c

create_socket.o: create_socket.c create_socket.h
	gcc $(CFLAGS) -c create_socket.c
	
real_address.o: real_address.c real_address.h
	gcc $(CFLAGS) -c real_address.c

wait_for_client.o: wait_for_client.c wait_for_client.h
	gcc $(CFLAGS) -c wait_for_client.c

test.o: test.c
	gcc $(CFLAGS) -c test.c

test: test.o read_write_loop.o create_socket.o real_address.o wait_for_client.o
	gcc $(CFLAGS) -o test test.o read_write_loop.o create_socket.o real_address.o wait_for_client.o -lcunit
	./test

clean:
	rm -rf *.o
	rm -rf chat
	rm -rf test
	rm -rf *.c~
	rm -rf *.h~

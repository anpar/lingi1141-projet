CFLAGS=-Wall -Werror -Wshadow -Wextra

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

clean:
	rm -rf *.o

mr-proper: clean
	rm -rf chat
	rm -rf *.c~
	rm -rf *.h~

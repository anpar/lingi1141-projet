#include <stdlib.h> 	/* EXIT_X, atoi */
#include <stdio.h> 		/* fprintf */
#include <getopt.h> 	/* getopt_long */
#include <unistd.h> 	/* close */

#include "real_address.h"
#include "create_socket.h"
#include "sender_core.h"

int main(int argc, char *argv[])
{
	int port;
	char *host;
	char * filename = NULL;

	int opt;
	int option_index = 0;
	struct option long_options[] = {
		{"filename",	required_argument,	0, 'f'},
		{0,				0,					0,  0 }
	};

	/* Check if -f/--filename option is present */
	while ((opt = getopt_long(argc, argv, "f:", long_options, &option_index)) != -1) {
		switch (opt) {
			case 'f':
			filename = optarg;
			break;
			default:
			fprintf(stderr, "Try './sender' for more information.\n");
			return EXIT_FAILURE;
			break;
		}
	}

	/* Get hostname and port */
	if((argc - optind) != 2) {
		fprintf(stderr, "Usage:\n"
		"\tsender [OPTION] HOSTNAME PORT\n"
		"Option:\n"
		"-f, --filename FILENAME\n"
		"\tsaves the data received into FILENAME\n");
		return EXIT_FAILURE;
	}

	host = argv[optind++];
	port = atoi(argv[optind++]);

	/* Resolve the hostname */
	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}

	/* Get a socket */
	int sfd = create_socket(NULL, -1, &addr, port); /* Connected */

	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}

	/* Process I/O */
	sender(sfd, filename);

	close(sfd);

	return EXIT_SUCCESS;
}

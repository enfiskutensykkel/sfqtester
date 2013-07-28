#include <tr1/memory>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <signal.h>
#include <getopt.h>
#include "barrier.h"
#include "streamer.h"
#include "socket.h"
#include <unistd.h>

using std::vector;
using std::tr1::shared_ptr;


static bool run = true;


static void terminate(void)
{
	run = false;
}



int main(int argc, char** argv)
{
	unsigned num_conns = 1;
	unsigned remote_port = 0;
	unsigned local_port = 0;
	unsigned interval = 0;
	unsigned length = 0;

	// Parse program arguments and options
	int opt;
	while ((opt = getopt(argc, argv, ":hc:p:q:n:i:")) != -1) {
		char* ptr;
		switch (opt) {

			// Missing a value for the option
			case ':':
				fprintf(stderr, "Option %s requires a value\n", argv[optind-1]);
				return ':';

			// Unknown option was given
			case '?':
				fprintf(stderr, "Ignoring unknown option '%c'\n", optopt);
				break;

			// Print program usage and quit
			case 'h':
				// TODO: Give usage
				return 'h';

			// Set number of connections
			case 'c':
				ptr = NULL;
				if ((num_conns = strtoul(optarg, &ptr, 0)) > 1024 || num_conns < 1 || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -c requires a valid number of connections [1-1024]\n");
					return 'c';
				}
				break;

			// Set the remote starting port
			case 'p':
				ptr = NULL;
				if ((remote_port = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -p requires a valid port number [0-65535]\n");
					return 'p';
				}
				break;

			// Set the local starting port
			case 'q':
				ptr = NULL;
				if ((local_port = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -q requires a valid port number [0-65535]\n");
					return 'p';
				}
				break;

			// Set the size of the byte chunk to be sent
			case 'n':
				ptr = NULL;
				if ((length = strtoul(optarg, &ptr, 0)) > BUFFER_SIZE || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -n requires a chunk size in bytes [1-%d] or 0 for off\n", BUFFER_SIZE);
					return 'n';
				}
				break;

			// Set the interval between each time a chunk is sent
			case 'i':
				ptr = NULL;
				if ((interval = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -i requires an interval in milliseconds [1-65535] or 0 for off\n");
					return 'i';
				}
				break;
		}
	}

	// Check if all mandatory options were set
	if (optind < argc && remote_port == 0) {
		fprintf(stderr, "Option -p is required for client\n");
		return 'p';
	} else if (optind == argc && local_port == 0) {
		fprintf(stderr, "Option -q is required for server\n");
		return 'q';
	}


	// Handle interrupt signal
	signal(SIGINT, (void (*)(int)) &terminate);

	// Create a barrier
	Barrier barrier(num_conns + 1);

	// Test streamer
	Server server(barrier, local_port);

	server.start();
	barrier.wait();

	while (run);

	return 0;
}

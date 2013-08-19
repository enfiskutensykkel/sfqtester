#include <tr1/memory>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include "barrier.h"
#include "stream.h"


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
	unsigned length = BUFFER_SIZE;
	unsigned duration = 0;

	// Handle interrupt signal
	signal(SIGINT, (void (*)(int)) &terminate);

	// Parse program arguments and options
	int opt;
	while ((opt = getopt(argc, argv, ":hc:p:q:n:i:t:")) != -1) 
	{
		char* ptr;
		switch (opt) 
		{

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
				if ((num_conns = strtoul(optarg, &ptr, 0)) > 1024 || num_conns < 1 || ptr == NULL || *ptr != '\0') 
				{
					fprintf(stderr, "Option -c requires a valid number of connections [1-1024]\n");
					return 'c';
				}
				break;

			// Set the remote starting port
			case 'p':
				ptr = NULL;
				if ((remote_port = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') 
				{
					fprintf(stderr, "Option -p requires a valid port number [0-65535]\n");
					return 'p';
				}
				break;

			// Set the local starting port
			case 'q':
				ptr = NULL;
				if ((local_port = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') 
				{
					fprintf(stderr, "Option -q requires a valid port number [0-65535]\n");
					return 'p';
				}
				break;

			// Set the size of the byte chunk to be sent
			case 'n':
				ptr = NULL;
				if ((length = strtoul(optarg, &ptr, 0)) > BUFFER_SIZE || ptr == NULL || *ptr != '\0') 
				{
					fprintf(stderr, "Option -n requires a chunk size in bytes [1-%d] or 0 for off\n", BUFFER_SIZE);
					return 'n';
				}
				break;

			// Set the interval between each time a chunk is sent
			case 'i':
				ptr = NULL;
				if ((interval = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') 
				{
					fprintf(stderr, "Option -i requires an interval in milliseconds [1-65535] or 0 for off\n");
					return 'i';
				}
				break;

			// Set the duration of the program
			case 't':
				ptr = NULL;
				duration = strtoul(optarg, &ptr, 0);
				if (ptr == NULL || *ptr != '\0') 
				{
					fprintf(stderr, "Option -t requires a duration in seconds, or 0 for off\n");
					return 't';
				}
				break;
		}
	}

	// Check if all mandatory options were set
	if (optind < argc && remote_port == 0) 
	{
		fprintf(stderr, "Option -p is required for client\n");
		return 'p';
	} 
	else if (optind == argc && local_port == 0) 
	{
		fprintf(stderr, "Option -q is required for server\n");
		return 'q';
	}


	vector< shared_ptr<Stream> > conns;

	// Create flows
	try
	{
		if (optind < argc)
		{
			// Create a barrier
			Barrier barrier(num_conns * (optind < argc) + 1);

			for (unsigned i = 0; i < num_conns; ++i) 
			{
				Client* client;

				if (local_port != 0)
					client = new Client(barrier, argv[optind], remote_port + i, local_port + i);
				else
					client = new Client(barrier, argv[optind], remote_port + i);

				client->set_chunk_size(length);
				client->set_interval(interval);
				client->start();

				conns.push_back(shared_ptr<Stream>(client));
			}

			// Synchronize with connections
			barrier.wait();  
		}
		else
		{
			conns.push_back(shared_ptr<Stream>( new Server(local_port, num_conns) ));
		}
	}
	catch (const char* exception)
	{
		// TODO: Better error handling, we probably don't want to stop everything if one of N threads/connections fail
		fprintf(stderr, "Unexpected error: %s\n", exception);
		run = false;
	}
	

	// Run until completion
	unsigned long time_left = duration * 1000;
	timespec timeout = {0, 1000000L}; 

	if (run && optind < argc && duration > 0) 
	{
		fprintf(stderr, "Running for %u seconds...\n", duration);
	} 
	else if (run && optind < argc) 
	{
		fprintf(stderr, "Running...\n");
	}

	while (run && !conns.empty()) 
	{

		// Sleep for a millisecond if a duration is set
		if (optind < argc && duration > 0) 
		{
			nanosleep(&timeout, NULL);

			// Check if the time is up
			if (--time_left == 0) 
			{
				break;
			}
		}
	}

	return 0;
}

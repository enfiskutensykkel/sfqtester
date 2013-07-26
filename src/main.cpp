#include <cstdlib>
#include <cstdio>
#include <signal.h>
#include <getopt.h>

int main(int argc, char** argv)
{
	unsigned num_conn = 1;
	unsigned rem_port = 0;
	unsigned loc_port = 0;
	unsigned interval = 0;
	unsigned length = 0;

	int opt;
	while ((opt = getopt(argc, argv, ":hc:p:q:n:i:")) != -1) {
		char* ptr;
		switch (opt) {
			case ':':
				fprintf(stderr, "Option %s requires a value\n", argv[optind-1]);
				return ':';

			case '?':
				fprintf(stderr, "Ignoring unknown option '%c'\n", optopt);
				break;

			case 'h':
				// TODO: Give usage
				return 'h';

			case 'c':
				ptr = NULL;
				if ((num_conn = strtoul(optarg, &ptr, 0)) > 1024 || num_conn < 1 || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -c requires a valid number of connections [1-1024]\n");
					return 'c';
				}
				break;

			case 'p':
				ptr = NULL;
				if ((rem_port = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -p requires a valid port number [0-65535]\n");
					return 'p';
				}
				break;

			case 'q':
				ptr = NULL;
				if ((loc_port = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -q requires a valid port number [0-65535]\n");
					return 'p';
				}
				break;

			case 'n':
				ptr = NULL;
				if ((length = strtoul(optarg, &ptr, 0)) > BUFFER_SIZE || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -n requires a chunk size in bytes [1-%d] or 0 for off\n", BUFFER_SIZE);
					return 'n';
				}
				break;

			case 'i':
				ptr = NULL;
				if ((interval = strtoul(optarg, &ptr, 0)) > 0xffff || ptr == NULL || *ptr != '\0') {
					fprintf(stderr, "Option -i requires an interval in milliseconds [1-65535] or 0 for off\n");
					return 'i';
				}
				break;
		}
	}

	if (optind < argc && rem_port == 0) {
		fprintf(stderr, "Option -p is required for client\n");
		return 'p';
	} else if (optind == argc && loc_port == 0) {
		fprintf(stderr, "Option -q is required for server\n");
		return 'q';
	}


	return 0;
}

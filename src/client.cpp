#include <tr1/cstdint>
#include <cstddef>
#include <cstdio>
#include <string>
#include <time.h>
#include "barrier.h"
#include "socket.h"
#include "stream.h"


Client::Client(Barrier& barrier, const char* host, uint16_t port) :
	Stream(),
	barr(barrier),
	hostname(host),
	rem_port(port),
	loc_port(0),
	buflen(BUFFER_SIZE),
	ival(0)
{
}


Client::Client(Barrier& barrier, const char* host, uint16_t r_port, uint16_t l_port) :
	Stream(),
	barr(barrier),
	hostname(host),
	rem_port(r_port),
	loc_port(l_port),
	buflen(BUFFER_SIZE),
	ival(0)
{
}


void Client::run()
{
	// Create connection to remote host
	Sock* sock;
   	
	if (loc_port > 0) {
		sock = Sock::create(hostname, rem_port, loc_port);
	} else {
		sock = Sock::create(hostname, rem_port);
	}

	// Couldn't create connection
	if (sock == NULL) {
		fprintf(stderr, "Couldn't connect to %s:%u\n", hostname, rem_port);
		barr.wait();
		return;
	}

	established = true;
	std::string host = sock->host();
	uint16_t port = sock->port();
	fprintf(stdout, "%s:%u Connected\n", host.c_str(), port);
	fflush(stdout);

	// Synchronize all threads
	barr.wait();

	// Send loop
	timespec timeout = {0, 1000000L};
	while (active && sock->alive()) {

		// Sleep for an interval
		uint64_t i = ival;
		while (active && i--) {
			nanosleep(&timeout, NULL);
		}

		ssize_t total, sent;
		total = buflen > 0 ? buflen : BUFFER_SIZE;
		sent = 0;

		// Send a chunk
		while (active && sock->alive() && total > 0) {
			sent = sock->write(buffer, total);
			if (sent < 0) {
				// Something went wrong
				break;
			}

			total -= sent;
		}
	}

	// Free socket
	established = false;
	fprintf(stdout, "%s:%u Closing connection\n", host.c_str(), port);
	fflush(stdout);
	delete sock;
}


void Client::set_chunk_size(size_t len)
{
	if (len <= BUFFER_SIZE) {
		buflen = len;
	}
}


void Client::set_interval(unsigned i)
{
	ival = i;
}

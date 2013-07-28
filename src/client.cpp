#include <tr1/cstdint>
#include <cstddef>
#include <cstdio>
#include <string>
#include <time.h>
#include "thread.h"
#include "barrier.h"
#include "socket.h"
#include "streamer.h"


Client::Client(Barrier& barrier, const char* host, uint16_t port) :
	Thread(),
	barr(barrier),
	hostname(host),
	rem_port(port),
	loc_port(0),
	buflen(BUFFER_SIZE),
	buf(NULL),
	ival(0),
	active(true)
{
	buf = new char[BUFFER_SIZE];
	for (unsigned i = 0; i < BUFFER_SIZE; ++i) {
		buf[i] = 'A';
	}
}


Client::~Client()
{
	stop();
	delete[] buf;
}


void Client::bind(uint16_t port)
{
	loc_port = port;
}


void Client::stop()
{
	active = false;
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

	std::string host = sock->host();
	uint16_t port = sock->port();
	fprintf(stdout, "Connected to %s:%u\n", host.c_str(), port);
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
		while (total > 0) {
			double time;
			sent = sock->write(buf, total, time);
			if (sent < 0) {
				// Something went wrong
				break;
			}

			total -= sent;
		}
	}

	// Free socket
	delete sock;
}

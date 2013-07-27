#include <tr1/cstdint>
#include <cstddef>
#include <cstdio>
#include "thread.h"
#include "barrier.h"
#include "socket.h"
#include "streamer.h"


Client::Client(Barrier& barrier, const char* host, uint16_t port) :
	Thread(),
	barr(barrier),
	sock(-1),
	hostname(host),
	rem_port(port),
	loc_port(0),
	buflen(BUFFER_SIZE),
	buf(NULL),
	ival(0),
	active(true)
{
	buf = new char[BUFFER_SIZE];
}


Client::~Client()
{
	stop();
	puts("stop");
	delete[] buf;
}


void Client::bind(uint16_t port)
{
}


void Client::stop()
{
	active = false;
}


#include <unistd.h>
void Client::run()
{
	// Create connection to remote host
	Sock* sock;
   	
	if (loc_port > 0) {
		sock = Sock::create(hostname, rem_port, loc_port);
	} else {
		sock = Sock::create(hostname, rem_port);
	}

	// Synchronize all threads
	barr.wait();

	while (active && sock->alive()) {
		sleep(2);
		break;
	}

	delete sock;
}

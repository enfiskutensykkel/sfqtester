#include <tr1/cstdint>
#include <tr1/memory>
#include <vector>
#include <cstddef>
#include <cstdio>
#include "thread.h"
#include "barrier.h"
#include "socket.h"
#include "streamer.h"

using std::vector;
using std::tr1::shared_ptr;


Server::Server(Barrier& barr, uint16_t port) :
	barr(barr),
	port(port),
	buf(NULL),
	active(true)
{
	buf = new char[BUFFER_SIZE];
}


Server::~Server()
{
	stop();
	delete[] buf;
}


void Server::stop()
{
	active = false;
}


void Server::run()
{
	// Create a server socket
	ListenSock* server = ListenSock::create(port);
	fprintf(stdout, "Listening on port %u\n", server->port());
	fflush(stdout);

	// Synchronize threads
	barr.wait();

	// Read loop
	while (active) {
		vector<shared_ptr<Sock> > socks = server->get_socks();

		for (unsigned i = 0; active && i < socks.size(); ++i) {
			
			// The socket was closed remotely
			if (!socks[i]->alive()) {
				continue;
			}

			// Read until everything is read
			ssize_t read;
			double time;
			while (active && (read = socks[i]->read(buf, BUFFER_SIZE, time)) != 0) {
				fprintf(stderr, "read %ld from %s:%u\n", read, socks[i]->host().c_str(), socks[i]->port());
			}
		}
	}

	// Free socket
	delete server;
}

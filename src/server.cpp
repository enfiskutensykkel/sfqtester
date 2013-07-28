#include <tr1/cstdint>
#include <tr1/memory>
#include <vector>
#include <cstddef>
#include <cstdio>
#include "barrier.h"
#include "socket.h"
#include "stream.h"

using std::vector;
using std::tr1::shared_ptr;


Server::Server(Barrier& barr, uint16_t port) :
	Stream(barr),
	port(port)
{
}


void Server::run()
{
	// Create a server socket
	ListenSock* server = ListenSock::create(port);

	if (server == NULL) {
		fprintf(stderr, "Couldn't listen on port %u\n", port);
		barr.wait();
		return;
	}

	established = true; // TODO: Make stream only established after having accepted at least one connection
	fprintf(stdout, "Listening on port %u\n", server->port());
	fflush(stdout);

	// Synchronize threads
	barr.wait();

	// Read loop
	while (active) {

		// Get a vector containing active connections
		vector<shared_ptr<Sock> > socks = server->get_socks();

		// Go through all of the active connections and read data
		for (unsigned i = 0; active && i < socks.size(); ++i) {
			
			// The socket was closed remotely
			if (!socks[i]->alive()) {
				continue;
			}

			// Read until everything is read
			ssize_t read;
			double time;
			while (active && (read = socks[i]->read(buffer, BUFFER_SIZE, time)) != 0) {
			}
		}
	}

	// Free socket
	established = false;
	fprintf(stdout, "Stopping listening on port %u\n", port);
	delete server;
}

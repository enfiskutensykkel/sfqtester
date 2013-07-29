#include <tr1/cstdint>
#include <tr1/memory>
#include <vector>
#include <set>
#include <cstddef>
#include <cstdio>
#include "barrier.h"
#include "socket.h"
#include "stream.h"



Server::Server(uint16_t port) :
	port(port)
{
}


void Server::run()
{
	using std::set;
	using std::vector;
	using std::tr1::shared_ptr;

	// Create a server socket
	ListenSock* server = ListenSock::create(port);

	if (server == NULL) {
		fprintf(stderr, "Couldn't listen on port %u\n", port);
		return;
	}

	established = true; // TODO: Make stream only established after having accepted at least one connection
	fprintf(stdout, "%u: Service started\n", server->port());
	fflush(stdout);

	// Create a map over active connections
	set<int> conn_set;

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

			// Check if we have a new connection
			set<int>::iterator it = conn_set.find(socks[i]->get_raw());
			if (it == conn_set.end()) {
				conn_set.insert(socks[i]->get_raw());
				fprintf(stdout, "%u: Connected to %s:%u\n", port, socks[i]->host().c_str(), socks[i]->port());
				fflush(stdout);
			}

			// Read until everything is read
			while (active && socks[i]->read(buffer, BUFFER_SIZE) > 0);
		}
	}

	// Free socket
	established = false;
	fprintf(stdout, "%u: Stopping service\n", port);
	delete server;
}

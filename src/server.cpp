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
	ListenSock* server = ListenSock::create(port);

	barr.wait();

	while (active) {
		vector<shared_ptr<Sock> > active = server->get_socks();

		double time;
		for (unsigned i = 0; i < active.size(); ++i) {
			while (active[i]->read(buf, BUFFER_SIZE, time) == 0) {

			}
		}
	}

	delete server;
}

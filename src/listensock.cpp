#include <sys/types.h>
#include <sys/socket.h>
#include <tr1/memory>
#include <tr1/cstdint>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include "socket.h"

using std::vector;
using std::tr1::shared_ptr;


static void close_sock(int* s)
{
	if (*s != -1) {
		::close(*s);
	}

	delete s;
}


/*
 * Listen for incoming connections.
 */
ListenSock::ListenSock(int sock) :
	listen_sock( new int(sock), &close_sock )
{
}


ListenSock* ListenSock::create(uint16_t port)
{
	addrinfo *ptr, *info = NULL;

	// Find info about the service/port
	if (!Sock::load_addrinfo(info, NULL, port)) {
		if (info != NULL) {
			freeaddrinfo(info);
		}
		return NULL;
	}

	int sock = -1;
	for (ptr = info; ptr != NULL; ptr = ptr->ai_next) {

		// Try to create socket descriptor
		if ((sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
			continue;
		}

		// Set port to be quickly reusable
		int flag = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

		// Try to bind to port and listen
		if (bind(sock, ptr->ai_addr, ptr->ai_addrlen) == 0 
				&& listen(sock, BACKLOG) == 0) {
			break;
		}
		
		// Failed, close socket and try again
		close(sock);
	}

	if (info) {
		freeaddrinfo(info);
	}

	// Couldn't listen for incoming connections
	if (ptr == NULL) {
		return NULL;
	}

	return new ListenSock(sock);
}


vector<shared_ptr<Sock> > ListenSock::get_socks()
{
	vector<shared_ptr<Sock> > list;
	return list;
}

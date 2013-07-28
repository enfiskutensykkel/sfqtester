#include <sys/types.h>
#include <sys/socket.h>
#include <tr1/cstdint>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include "socket.h"

using std::vector;
using std::tr1::shared_ptr;


/*
 * Listen for incoming connections.
 */
ListenSock::ListenSock(int sock)
{
	listen_sock = sock;

	// Initialize descriptor set
	FD_ZERO(&all_fds);
	FD_SET(listen_sock, &all_fds);
	hi_sock = listen_sock;
}


ListenSock::~ListenSock()
{
	::close(listen_sock);
	FD_ZERO(&all_fds);
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
	vector<shared_ptr<Sock> > active_list;
	fd_set active = all_fds;


	// Select the number of descriptors with any events
	timeval wait = {0, 0};
	int num_active = select(hi_sock + 1, &active, NULL, NULL, &wait);
	if (num_active == -1) {
		// TODO: Error handling for select returning -1
		return active_list;
	}

	// Accept any incomming connections
	if (FD_ISSET(listen_sock, &active)) {
		int sock = accept(listen_sock, NULL, NULL);

		if (sock != -1) {
			// Add the new descriptor to the descriptor set
			FD_SET(sock, &all_fds);
			hi_sock = sock > hi_sock ? sock : hi_sock;
			socks.push_back(shared_ptr<Sock>( new Sock(sock) ));
		}
		// TODO: Error handling for accept returning -1
		
		--num_active;
	}


	// Iterate over all connections and find the ones that has any events
	vector<shared_ptr<Sock> >::iterator it = socks.begin();
	while (num_active > 0 && it != socks.end()) {

		shared_ptr<Sock> sock = *it;

		// Found active descriptor
		if (*sock->sock != -1 && FD_ISSET(*sock->sock, &active)) {
			active_list.push_back(sock);
			--num_active;
		} 

		++it;
	}

	return active_list;
}


/*
 * Get the port we're listening on.
 */
uint16_t ListenSock::port()
{
	if (listen_sock) {
		sockaddr_in addr;
		socklen_t len = sizeof(addr);

		// Load info about the socket descriptor
		if (getsockname(listen_sock, reinterpret_cast<sockaddr*>(&addr), &len) == -1) {
			return 0;
		}

		return ntohs(addr.sin_port);
	}

	return 0;
}

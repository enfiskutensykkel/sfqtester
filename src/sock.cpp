#include <sys/types.h>
#include <sys/socket.h>
#include <tr1/cstdint>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include "socket.h"


/*
 * Try to bind a socket descriptor to a local port.
 */
static inline
bool bind_port(int sock, uint16_t port)
{
	sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	// Set the port to quickly reusable, so subsequent connections can quickly
	// reuse the port
	int flag = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	return bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}


static void close_sock(int* s)
{
	if (*s != -1) {
		::close(*s);
	}

	delete s;
}


/*
 * Give control of a socket descriptor to an instance of socket.
 */
Sock::Sock(int descriptor) :
	sock( new int(descriptor), &close_sock )
{
}


/*
 * Get the socket descriptor.
 */
int Sock::get_fd()
{
	return *sock;
}


/*
 * Check if the socket descriptor is still valid.
 */
bool Sock::alive()
{
	// TODO: Copy the validation from streamzero_client_streams
	return *sock != -1;
}


/*
 * Create a TCP connection to a remote host.
 */
Sock* Sock::create(const char* host, uint16_t remote_port, uint16_t local_port)
{
	addrinfo *ptr, *info = NULL;

	// Find info about remote host
	if (!load_addrinfo(info, host, remote_port)) {
		if (info != NULL) {
			freeaddrinfo(info);
		}

		return NULL;
	}

	int sock = -1;
	for (ptr = info; ptr != NULL; ptr = ptr->ai_next) {

		// Try to create a socket descriptor
		if ((sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
			continue;
		}

		// Turn off Nagle's algorithm
		int flag = 1;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

		// If a local port is given, try to bind to that port
		if (local_port > 0) {
			bind_port(sock, local_port);
		}

		// Try to connect to remote host
		if (connect(sock, ptr->ai_addr, ptr->ai_addrlen) != -1) {
			break;
		}

		// Failed, close the socket and try again
		close(sock);
	}

	if (info) {
		freeaddrinfo(info);
	}

	// No suitable connection could be made
	if (ptr == NULL) {
		return NULL;
	}

	return new Sock(sock);
}


/*
 * Create a TCP connection to a remote host using a random outgoing port.
 */
Sock* Sock::create(const char* hostname, uint16_t port)
{
	return create(hostname, port, 0);
}

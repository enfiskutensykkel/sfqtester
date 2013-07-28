#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <sstream>
#include <tr1/cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "socket.h"

using std::string;


/*
 * Find out the hostname and port number of the remote end of a connection.
 */
static inline
bool load_info(int sock, string& name, uint16_t& port)
{
	sockaddr_in addr;

	// Load info about other end (the peer)
	socklen_t len = sizeof(addr);
	if (getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &len) == -1) {
		return false;
	}

	// Get hold of the hostname
	char nname[INET_ADDRSTRLEN];
	if (getnameinfo(reinterpret_cast<sockaddr*>(&addr), (socklen_t) sizeof(addr), nname, sizeof(nname), NULL, 0, NI_NUMERICHOST) != 0) {
		return false;
	}

	name = string(nname);

	// Get the port number
	port = ntohs(addr.sin_port);

	return true;
}


/*
 * Allocate and fill a struct addrinfo.
 */
bool Sock::load_addrinfo(addrinfo*& info, const char* host, uint16_t port)
{
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = host != NULL ? AI_PASSIVE : 0;

	std::ostringstream converter;
	converter << port; // convert uint16_t to string

	return getaddrinfo(host, converter.str().c_str(), &hints, &info) == 0;
}


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
	// FIXME: Set the descriptor to non-blocking
}


/*
 * Check if the socket descriptor is still valid.
 */
bool Sock::alive()
{
	if (*sock == -1 || (fcntl(*sock, F_GETFL) == -1 && errno == EBADF)) {
		*sock = -1;
		return false;
	}

	return true;
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


/*
 * Read data from the connection.
 */
ssize_t Sock::read(char* buf, size_t len, double& time)
{
	if (*sock != -1) {
		ssize_t bytes = recv(*sock, buf, len, MSG_DONTWAIT);

		if (bytes <= 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			return 0;
		} else if (bytes == 0) {
			// Remote end closed the connection
			close(*sock);
			*sock = -1;
			return -1;
		} else if (bytes == -1) {
			// TODO: Error handling
			return -1;
		}

		return bytes;
	}

	return -1;
}


/*
 * Write data to a connection
 */
ssize_t Sock::write(const char* buf, size_t len, double& time)
{
	if (*sock != -1) {
		ssize_t bytes = send(*sock, buf, len, MSG_DONTWAIT);

		if (bytes == -1) {
			close(*sock);
			*sock = -1;
			return -1;
		}

		return bytes;
	}

	return -1;
}


/*
 * Get remote port.
 */
uint16_t Sock::port()
{
	string name;
	uint16_t port = 0;

	if (*sock != -1) {
		load_info(*sock, name, port);
	}

	return port;
}


/*
 * Get remote hostname
 */
string Sock::host()
{
	string name;
	uint16_t port;

	if (*sock != -1) {
		load_info(*sock, name, port);
	}

	return name;
}

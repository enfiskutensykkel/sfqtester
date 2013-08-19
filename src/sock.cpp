#include "sock.h"
#include <sstream>
#include <string>
#include <tr1/cstdint>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>


using std::string;



static inline
bool load_peerinfo(int sock, string& host, uint16_t& port)
{
	sockaddr_in addr;
	socklen_t len;

	// Load info about the other end (the peer)
	len = sizeof(addr);
	if (getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &len) == -1)
		return false;

	// Extract the hostname and remote port
	char name[INET_ADDRSTRLEN];
	if (getnameinfo(reinterpret_cast<sockaddr*>(&addr), (socklen_t) sizeof(addr), name, sizeof(name), NULL, 0, NI_NUMERICHOST) != 0)
		return false;

	host = std::string(name);
	port = ntohs(addr.sin_port);

	return true;
}



static inline
bool load_addrinfo(addrinfo*& info, const char* host, uint16_t port)
{
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = host == NULL ? AI_PASSIVE : 0;

	std::ostringstream converter;
	converter << port;

	return getaddrinfo(host, converter.str().c_str(), &hints, &info) == 0;
}



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



int Sock::create(uint16_t port)
{
	addrinfo *ptr, *info = NULL;

	// Get info about the service/port
	if (!load_addrinfo(info, NULL, port))
	{
		if (info != NULL)
			freeaddrinfo(info);

		return -1;
	}

	int sock = -1;
	for (ptr = info; ptr != NULL; ptr = ptr->ai_next)
	{
		// Try to create a socket descriptor
		if ((sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
			continue;

		// Set port to be reusable
		int flag = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

		// Try to bind to the port and listen
		if (bind(sock, ptr->ai_addr, ptr->ai_addrlen) == 0
				&& listen(sock, SOMAXCONN) == 0)
			break;

		// Failed, close socket and try again
		::close(sock);
	}

	if (info != NULL)
		freeaddrinfo(info);

	if (ptr == NULL)
		return -1;

	return sock;
}



int Sock::create(const char* host, uint16_t rem_port, uint16_t loc_port)
{
	addrinfo *ptr, *info = NULL;

	// Get info about the service/port
	if (!load_addrinfo(info, host, rem_port))
	{
		if (info != NULL)
			freeaddrinfo(info);

		return -1;
	}

	int sock = -1;
	for (ptr = info; ptr != NULL; ptr = ptr->ai_next)
	{
		// Try to create a socket descriptor
		if ((sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
			continue;

		// Turn off Nagle's algorithm
		int flag = 1;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));


		// If a local port is given, try to bind to that port
		if (loc_port != 0)
		{
			bind_port(sock, loc_port);
		}

		// Try to connect to the remote host
		if (connect(sock, ptr->ai_addr, ptr->ai_addrlen) != -1)
			break;

		// Failed, close socket and try again
		::close(sock);
	}

	if (info != NULL)
		freeaddrinfo(info);

	if (ptr == NULL)
		return -1;

	return sock;
}



int Sock::create(const char* host, uint16_t port)
{
	return create(host, port, 0);
}



static
void close_sock(int* sfd)
{
	if (*sfd != -1)
		::close(*sfd);

	delete sfd;
}



Sock::Sock(int fd)
	: sfd( new int(fd), &close_sock )
{
	// TODO: Check if valid socket descriptor

	// Set descriptor to non-blocking
	if (*sfd != -1)
	{
		int flag = fcntl(*sfd, F_GETFL, 0);
		fcntl(*sfd, F_SETFL, flag | O_NONBLOCK);
	}
}



bool Sock::connected()
{
	if (*sfd != -1)
	{
		// TODO: Check if descriptor is still alive
		int flag = 0;
		socklen_t len = sizeof(flag);
		getsockopt(*sfd, SOL_SOCKET, SO_ACCEPTCONN, &flag, &len);
		return flag != 0;

	}
	else
	{
		return false;
	}
}



int Sock::raw()
{
	return *sfd;
}



bool Sock::peer(string& name)
{
	uint16_t port;
	return *sfd != -1 && load_peerinfo(*sfd, name, port);
}



bool Sock::peer(uint16_t& port)
{
	string name;
	return *sfd != -1 && load_peerinfo(*sfd, name, port);
}



bool Sock::host(uint16_t& port)
{
	if (*sfd != -1)
	{
		sockaddr_in addr;

		socklen_t len = sizeof(addr);
		if (getsockname(*sfd, reinterpret_cast<sockaddr*>(&addr), &len) == -1)
			return false;

		port = ntohs(addr.sin_port);
		return true;
	}
	else
	{
		return false;
	}
}



void Sock::close()
{
	if (*sfd != -1)
	{
		::close(*sfd);
		*sfd = -1;
	}
}

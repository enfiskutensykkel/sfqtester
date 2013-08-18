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
#include <cstdio>


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



Sock Sock::create(uint16_t port)
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
	int flag = fcntl(*sfd, F_GETFL, 0);
	fcntl(*sfd, F_SETFL, flag | O_NONBLOCK);
}



bool Sock::connected()
{
	int flag = 0;
	socklen_t len = sizeof(flag);
	getsockopt(*sfd, SOL_SOCKET, SO_ACCEPTCONN, &flag, &len);
	return flag != 0;
}



int Sock::raw()
{
	return *sfd;
}



bool Sock::peer(string& name)
{
	uint16_t port;
	return load_peerinfo(*sfd, name, port);
}



bool Sock::peer(uint16_t& port)
{
	string name;
	return load_peerinfo(*sfd, name, port);
}



bool Sock::host(uint16_t& port)
{
	sockaddr_in addr;

	socklen_t len = sizeof(addr);
	if (getsockname(*sfd, reinterpret_cast<sockaddr*>(&addr), &len) == -1)
		return false;

	port = ntohs(addr.sin_port);
	return true;
}



void Sock::close()
{
	if (*sfd != -1)
	{
		::close(*sfd);
		*sfd = -1;
	}
}

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <tr1/cstdint>
#include <tr1/memory>
#include <vector>
#include <netdb.h>

class Sock;


/*
 * Socket descriptor wrapper
 *
 * Manages multiple connections from remote hosts.
 */
class ListenSock 
{

	public:
	
		/*
		 * Get a vector containing the sockets that have something interesting
		 * to report.
		 */	
		std::vector<std::tr1::shared_ptr<Sock> > get_socks(void);

		/* 
		 * Create a socket descriptor to listen for incomming connections.
		 */
		static ListenSock* create(uint16_t port);

	private:
		std::tr1::shared_ptr<int> listen_sock;
		ListenSock(int socket_descriptor);
};



/*
 * Socket descriptor wrapper
 *
 * Manages a connection to a remote host.
 */
class Sock
{
	public:

		/*
		 * Create a new instance.
		 */
		Sock(int socket_descriptor);

		/*
		 * Get the raw socket descriptor.
		 */
		int get_fd(void);

		/*
		 * Validate that the descriptor is still valid and that the connection
		 * is still alive.
		 */
		bool alive(void);

		/*
		 * Create a TCP connection to a server on a remote machine using a 
		 * bound outgoing port.
		 */
		static Sock* create(
				const char* hostname, 
				uint16_t remote_port, 
				uint16_t local_port
				);

		/*
		 * Create a TCP connection to a server on a remote machine using a 
		 * random outgoing port.
		 */
		static Sock* create(
				const char* hostname, 
				uint16_t port
				);

	private:
		std::tr1::shared_ptr<int> sock;
};



/*
 * Allocate and fill a struct addrinfo with information about a host and/or 
 * service running on a specific port.
 */
bool load_addrinfo(addrinfo*& addr_info, const char* hostname, uint16_t port);

#endif

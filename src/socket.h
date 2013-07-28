#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <cstddef>
#include <tr1/cstdint>
#include <tr1/memory>
#include <vector>
#include <string>
#include <netdb.h>
#include <sys/select.h>

class Sock;


/*
 * Socket descriptor wrapper
 *
 * Manages multiple connections from remote hosts.
 */
class ListenSock 
{
	public:
		~ListenSock(void);
	
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
		std::vector<std::tr1::shared_ptr<Sock> > socks;
		int listen_sock;
		fd_set all_fds;
		int hi_sock, num_active;
		ListenSock(int socket_descriptor);

		ListenSock& operator=(const ListenSock& other);
		ListenSock(const ListenSock& other);
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
		 * Get the numerical hostname of the remote host.
		 */
		std::string remote_host(void);

		/*
		 * Get the port number of the remote end of the connection.
		 */
		uint16_t remote_port(void);

		/*
		 * Read data from a connection into a buffer, and get the time
		 * elapsed since the last read operation.
		 */
		ssize_t read(char* buffer, size_t buffer_length, double& elapsed);

		/*
		 * Write data to a connection from a buffer, and get the time elapsed
		 * since the last write operation.
		 */
		ssize_t write(const char* buffer, size_t buffer_length, double& elapsed);

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
		friend class ListenSock;

		std::tr1::shared_ptr<int> sock;

		/* Create new instance */
		Sock(int socket_descriptor);

		/*
		 * Allocate and fill a struct addrinfo with information about a host 
		 * and/or service running on a specific port.
		 */
		static bool load_addrinfo(
				addrinfo*& addr_info, 
				const char* hostname, 
				uint16_t port
				);
};

#endif

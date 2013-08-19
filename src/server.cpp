#include "stream.h"
#include "sock.h"
#include <map>
#include <cstdlib>
#include <string>
#include <tr1/cstdint>
#include <cstdio>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

using std::map;



void Server::accepter_thread(Server* server)
{
	epoll_event* events = server->events;
	int thread = 0; // round robin scheduling

	uint16_t remote_port, local_port;
	std::string hostname;

	while (server->state == RUNNING)
	{
		int num_evts = epoll_wait(server->lfd, events, server->conns, 5);
		if (num_evts == -1)
			break;

		for (int i = 0; i < num_evts; ++i)
		{
			// Check for problems with epoll
			if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) || !(events[i].events & EPOLLIN))
			{
				server->socks.erase(events[i].data.fd);
				continue;
			}

			while (true)
			{
				// Accept incomming connections
				int sock = accept(events[i].data.fd, NULL, NULL);

				// We have a connection
				if (sock != -1)
				{
					// Add to epoll descriptor for a thread
					epoll_event evt = {0, {0}};
					evt.data.fd = sock;
					evt.events = EPOLLIN;
					epoll_ctl(server->cfd[thread], EPOLL_CTL_ADD, sock, &evt);

					// Update thread index
					thread = (thread + 1) % RECV_THREADS;

					// Add connection to the map of connections
					Sock socket(sock);
					server->socks.insert(std::pair<int,Sock>(sock, socket));

					// Output connection information
					socket.peer(hostname);
					socket.peer(remote_port);
					socket.host(local_port);
					fprintf(stdout, "%u: Connected to %s:%u (%d)\n", local_port, hostname.c_str(), remote_port, sock);
					fflush(stdout);
				}
				else
				{
					break;
				}
			}
		}
	}

	pthread_exit(NULL);
}



void Server::receiver_thread(Server* server)
{
	// Get thread number
	int idx;
	for (idx = 0; idx < RECV_THREADS; ++idx)
	{
		if (server->receivers[idx] == pthread_self())
			break;
	}

	epoll_event events[BACKLOG];
	char buffer[BUFFER_SIZE];
	uint16_t remote_port, local_port;
	std::string hostname;

	// Main loop
	while (server->state == RUNNING)
	{
		int num_evts = epoll_wait(server->cfd[idx], events, BACKLOG, 2);
		if (num_evts == -1)
			goto break_out;

		for (int i = 0; i < num_evts; ++i)
		{
			// Get connection information
			map<int,Sock>::iterator sock = server->socks.find(events[i].data.fd);
			sock->second.peer(hostname);
			sock->second.peer(remote_port);
			sock->second.host(local_port);

			// Check for problems with epoll
			if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) || !(events[i].events & EPOLLIN))
			{
				server->socks.erase(sock);
				fprintf(stdout, "%u: Connection to %s:%u broken (%d)\n", local_port, hostname.c_str(), remote_port, events[i].data.fd);
				fflush(stdout);
				continue;
			}


			// Read from socket descriptor
			ssize_t bytes;
			while (server->state == RUNNING && (bytes = read(events[i].data.fd, buffer, sizeof(buffer))) == BUFFER_SIZE);

			if (bytes == 0)
			{
				// Connection closed by peer
				server->socks.erase(sock);
				fprintf(stdout, "%u: Closing connection to %s:%u (%d)\n", local_port, hostname.c_str(), remote_port, events[i].data.fd);
				fflush(stdout);
			}
			else if (bytes == -1 && errno != EAGAIN)
			{
				// Something is wrong
				server->socks.erase(sock);
				fprintf(stdout, "%u: Connection to %s:%u broken (%d)\n", local_port, hostname.c_str(), remote_port, events[i].data.fd);
				fflush(stdout);
				goto break_out;
			}
		}
	}

break_out:
	pthread_exit(NULL);
}



Server::Server(uint16_t port, unsigned conns)
	: state(RUNNING), conns(conns), lfd(-1), events(NULL)
{
	pthread_attr_t attr;

	// Create thread attributes
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 16000);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	// Create epoll descriptors
	if ((lfd = epoll_create1(0)) == -1)
		throw "Couldn't create epoll instance";

	for (unsigned i = 0; i < RECV_THREADS; ++i)
	{
		if ((cfd[i] = epoll_create1(0)) == -1)
			throw "Couldn't create epoll instance";
	}

	if ((events = static_cast<epoll_event*>(calloc(conns, sizeof(epoll_event)))) == NULL)
		throw "Couldn't allocate enough memory";


	// Create listen sockets
	for (unsigned i = 0; i < conns; ++i)
	{
		epoll_event ev = {0, {0}}; // Valgrind complains about unitialized bytes
		fprintf(stdout, "%u: Starting service\n", port + i);
		fflush(stdout);

		Sock sock = Sock::create(port + i);

		ev.data.fd = sock.raw();
		ev.events = EPOLLIN;

		if (epoll_ctl(lfd, EPOLL_CTL_ADD, sock.raw(), &ev) == -1)
			throw "Failed to add descriptor to epoll set";

		socks.insert(std::pair<int,Sock>(sock.raw(), sock));
	}
	
	// Try to create accept thread
	if (pthread_create(&accepter, &attr, (void* (*)(void*)) &accepter_thread, (void*) this) != 0)
		throw "Failed to create accepter thread";

	// Try to create reader threads
	for (int i = 0; i < RECV_THREADS; ++i)
	{
		if (pthread_create(&receivers[i], &attr, (void* (*)(void*)) &receiver_thread, (void*) this) != 0)
		{
			throw "Failed to create receiver thread";
		}
	}

	pthread_attr_destroy(&attr);
}



Server::~Server()
{
	state = STOPPING;

	for (int i = 0; i < RECV_THREADS; ++i)
		pthread_join(receivers[i], NULL);

	pthread_join(accepter, NULL);

	for (map<int,Sock>::iterator it = socks.begin(); it != socks.end(); ++it)
	{
		if (it->second.connected())
			it->second.close();
	}
	socks.clear();

	free(events);
}

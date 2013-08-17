#include "stream.h"
#include <cstdlib>
#include <string>
#include <tr1/cstdint>
#include <cstdio>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>


void Server::accepter_thread(Server* server)
{
	epoll_event* events = server->events;
	int thread = 0;

	while (server->state == RUNNING)
	{
		int num_evts = epoll_wait(server->lfd, events, server->conns, 5);
		if (num_evts == -1)
			break;

		for (int i = 0; i < num_evts; ++i)
		{
			if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) || !(events[i].events & EPOLLIN))
			{
				// TODO: Notify that the socket is broken
				close(events[i].data.fd);
				continue;
			}

			while (true)
			{
				int sock = accept(events[i].data.fd, NULL, NULL);

				if (sock != -1)
				{
					int flag = fcntl(sock, F_GETFL, 0);
					fcntl(sock, F_SETFL, flag | O_NONBLOCK);

					epoll_event evt = {0, {0}};
					evt.data.fd = sock;
					evt.events = EPOLLIN;

					epoll_ctl(server->cfd[thread], EPOLL_CTL_ADD, sock, &evt);
					thread = (thread + 1) % RECV_THREADS;
					
					// Load info about the connection
					std::string name;
					uint16_t lport, rport;
					server->remote(sock, name, rport);
					server->local(sock, lport);
					fprintf(stdout, "%u: Connected to %s:%u\n", lport, name.c_str(), rport);
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



void Server::reader_thread(Server* server)
{
	// Get thread number
	int idx;
	for (idx = 0; idx < RECV_THREADS; ++idx)
	{
		if (server->readers[idx] == pthread_self())
			break;
	}

	epoll_event events[BACKLOG];
	char buffer[BUFFER_SIZE];

	// Main loop
	while (server->state == RUNNING)
	{
		int num_evts = epoll_wait(server->cfd[idx], events, BACKLOG, 2);
		if (num_evts == -1)
			goto break_out;

		for (int i = 0; i < num_evts; ++i)
		{
			if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) || !(events[i].events & EPOLLIN))
			{
				uint16_t port;
				server->local(events[i].data.fd, port);
				fprintf(stdout, "%u: Broken connection\n", port);
				close(events[i].data.fd);
				continue;
			}


			ssize_t bytes;
			while (server->state == RUNNING && (bytes = read(events[i].data.fd, buffer, sizeof(buffer))) == BUFFER_SIZE);

			if (bytes == 0)
			{
				std::string name;
				uint16_t lport, rport;
				server->remote(events[i].data.fd, name, rport);
				server->local(events[i].data.fd, lport);
				close(events[i].data.fd);

				fprintf(stdout, "%u: Disconnecting from %s:%u\n", lport, name.c_str(), rport);
			}
			else if (bytes == -1 && errno != EAGAIN)
			{
				uint16_t port;
				server->local(events[i].data.fd, port);
				fprintf(stdout, "%u: Broken connection\n", port);
				close(events[i].data.fd);
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
		int sock = Stream::sock(port + i);

		ev.data.fd = sock;
		ev.events = EPOLLIN;

		if (epoll_ctl(lfd, EPOLL_CTL_ADD, sock, &ev) == -1)
			throw "Failed to add descriptor to epoll set";
	}
	
	// Try to create accept thread
	if (pthread_create(&accepter, &attr, (void* (*)(void*)) accepter_thread, (void*) this) != 0)
		throw "Failed to create accepter thread";

	// Try to create reader threads
	for (int i = 0; i < RECV_THREADS; ++i)
	{
		if (pthread_create(&readers[i], &attr, (void* (*)(void*)) reader_thread, (void*) this) != 0)
		{
			// TODO: Handle error
		}
	}

	pthread_attr_destroy(&attr);
}



Server::~Server()
{
	state = STOPPING;

	for (int i = 0; i < RECV_THREADS; ++i)
		pthread_join(readers[i], NULL);

	pthread_join(accepter, NULL);

	free(events);
}

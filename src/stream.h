#ifndef __STREAMER_H__
#define __STREAMER_H__

#include "sock.h"
#include "barrier.h"
#include <map>
#include <tr1/cstdint>
#include <pthread.h>
#include <sys/epoll.h>



class Stream
{
	public:
		virtual ~Stream(void);
};



class Server: public Stream
{
	public:
		Server(uint16_t start_port, unsigned expected_conns);
		~Server(void);

	private:
		enum {RUNNING, STOPPING} state;
		unsigned conns;
		pthread_t accepter;
	   	pthread_t receivers[RECV_THREADS];

		std::map<int, Sock> socks;

		int lfd, cfd[RECV_THREADS];
		epoll_event* events;

		static void accepter_thread(Server*);
		static void receiver_thread(Server*);
};



class Client: public Stream
{
	public:
		Client(Barrier& barrier, const char* hostname, uint16_t remote_port, uint16_t local_port = 0);
		~Client(void);
	
	private:
		enum { STARTED, STOPPED } state;
		Barrier& barrier;
		Sock sock;
		pthread_t id;

		static void run(Client*);
};

#endif

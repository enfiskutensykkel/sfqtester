#ifndef __STREAMER_H__
#define __STREAMER_H__

#include "sock.h"
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

#endif

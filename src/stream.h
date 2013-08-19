#ifndef __STREAMER_H__
#define __STREAMER_H__

#include "sock.h"
#include "barrier.h"
#include <map>
#include <cstddef>
#include <tr1/cstdint>
#include <pthread.h>
#include <sys/epoll.h>



class Stream
{
	public:
		virtual ~Stream(void);
		virtual bool active(void);
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

		bool set_chunk_size(size_t bytes);
		bool set_interval(unsigned ms);

		void start(void);
		void stop(void);

		bool active(void);
	
	private:
		enum { INIT, STARTED, RUNNING, STOPPED } state;
		pthread_t id;
		pthread_mutex_t mutex;
		pthread_cond_t start_signal;
		pthread_cond_t stop_signal;

		Barrier& barrier;

		const char* hostname;
		uint16_t remote_port;
		uint16_t local_port;

		char* buf;
		size_t buflen;
		unsigned ival;
		bool _active;

		void run(void);
		static void dispatch(Client*);
};

#endif

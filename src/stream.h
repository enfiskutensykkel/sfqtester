#ifndef __STREAMER_H__
#define __STREAMER_H__

#include <tr1/cstdint>
#include <pthread.h>
#include <vector>
#include <sys/epoll.h>


class Stream
{
	public:
		virtual ~Stream(void) = 0;

	protected:
		static int sock(const char* hostname, uint16_t remote_port, uint16_t local_port);
		static int sock(const char* hostname, uint16_t port);
		static int sock(uint16_t port);
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
	   	pthread_t readers[RECV_THREADS];

		int lfd, cfd[RECV_THREADS];
		epoll_event* events;
		
		static void accepter_thread(Server*);
		static void reader_thread(Server*);
};

#endif

#ifndef __STREAMER_H__
#define __STREAMER_H__

#include <vector>
#include <tr1/cstdint>
#include <cstddef>
#include "thread.h"


class Barrier;


class Client : public Thread
{
	public:
		Client(Barrier& barrier, const char* hostname, uint16_t port);
		~Client(void);

		void bind(uint16_t local_port);
		void set_chunk_size(size_t bytes);
		void set_interval(unsigned ms);

		void stop(void);
		void run(void);

	private:
		Barrier& barr;
		int sock;
		const char* hostname;
		uint16_t rem_port;
		uint16_t loc_port;
		size_t buflen;
		char* buf;
		unsigned ival;
		bool active;
};


class Server : public Thread
{
	public:
		Server(Barrier& barrier, uint16_t port);
		~Server(void);

		void stop(void);
		void run(void);

	private:
		Barrier& barr;
		std::vector<int> socks;
		uint16_t port;
		bool active;
};

#endif

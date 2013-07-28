#ifndef __STREAMER_H__
#define __STREAMER_H__

#include <tr1/cstdint>
#include <cstddef>
#include "thread.h"


class Barrier;


class Stream : public Thread
{
	public:
		Stream(void);
		virtual ~Stream(void);

		virtual bool is_active(void);
		void stop(void);
		virtual void run(void) = 0;

	protected:
		char* buffer;
		bool active;
		bool established;
};


class Client : public Stream
{
	public:
		Client(Barrier& barrier, const char* hostname, uint16_t port);
		Client(Barrier& barrier, const char* hostname, uint16_t remote_port, uint16_t local_port);

		void set_chunk_size(size_t bytes);
		void set_interval(unsigned ms);

		void run(void);

	private:
		Barrier& barr;
		const char* hostname;
		uint16_t rem_port, loc_port;
		size_t buflen;
		unsigned ival;
};


class Server : public Stream
{
	public:
		Server(uint16_t port);

		void run(void);

	private:
		uint16_t port;
};

#endif

#ifndef __STREAMER_H__
#define __STREAMER_H__

#include <tr1/cstdint>

class Stream
{
	public:
		Stream(void);
		virtual ~Stream(void);
};



class Server
{
	public:
		Server(uint16_t port, unsigned expected_conns);
		~Server(void);

		
};

#endif

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <tr1/cstdint>
#include <tr1/memory>
#include <string>


class Sock
{
	public:
		int raw(void);
		bool connected(void);
		void close(void);

		bool peer(std::string& hostname);
		bool peer(uint16_t& port);
		bool host(uint16_t& port);

		static int create(uint16_t port);
		static int create(const char* hostname, uint16_t remote_port);
		static int create(const char* hostname, uint16_t remote_port, uint16_t local_port);

		Sock(int sockfd);

	private:
		std::tr1::shared_ptr<int> sfd;
};

#endif

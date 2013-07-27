#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "socket.h"


bool load_addrinfo(addrinfo*& info, const char* host, uint16_t port)
{
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = host != NULL ? AI_PASSIVE : 0;

	std::ostringstream converter;
	converter << port; // convert uint16_t to string

	return getaddrinfo(host, converter.str().c_str(), &hints, &info) == 0;
}

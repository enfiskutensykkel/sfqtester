#include "barrier.h"
#include "thread.h"
#include "stream.h"


static char buffer[BUFFER_SIZE];

Stream::Stream() :
	Thread(),
	buffer(::buffer),
	active(true),
	established(false)
{
}


Stream::~Stream()
{
	stop();
}


void Stream::stop()
{
	active = false;
}


bool Stream::is_active()
{
	return active && established; // TODO: Override this in client and server
}

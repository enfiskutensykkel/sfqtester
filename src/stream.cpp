#include "barrier.h"
#include "thread.h"
#include "stream.h"


Stream::Stream() :
	Thread(),
	buffer(NULL),
	active(true),
	established(false)
{
	buffer = new char[BUFFER_SIZE];
	for (unsigned i = 0; i < BUFFER_SIZE; ++i) {
		buffer[i] = 'A';
	}
}


Stream::~Stream()
{
	stop();
	delete[] buffer;
}


void Stream::stop()
{
	active = false;
}


bool Stream::is_active()
{
	return active && established; // TODO: Override this in client and server
}

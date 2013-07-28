#include "barrier.h"
#include "thread.h"
#include "stream.h"


Stream::Stream(Barrier& barrier) :
	Thread(),
	barr(barrier),
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
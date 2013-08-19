#include "stream.h"
#include "sock.h"
#include "barrier.h"
#include <pthread.h>



Client::Client(Barrier& barr, const char* host, uint16_t rem_port, uint16_t loc_port)
	: state(STARTED), barrier(barr), sock(-1)
{
	// Create thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 16000);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	// Create socket descriptor
	if (loc_port != 0)
		sock = Sock::create(host, rem_port, loc_port);
	else
		sock = Sock::create(host, rem_port);

	if (sock.raw() == -1)
	{
		throw "Failed to create socket descriptor";
	}

	// Try to create thread
	if (pthread_create(&id, &attr, (void* (*)(void*)) &run, (void*) this) != 0)
	{
		// Failed to create thread
		// TODO: Better error handling
		throw "Failed to create thread";
	}

	// Clean up
	pthread_attr_destroy(&attr);
}



Client::~Client()
{
	if (state != STOPPED)
	{
		state = STOPPED;
		pthread_join(id, NULL);
	}
}




void Client::run(Client* client)
{
	// Wait on the barrier
	client->barrier.wait();

	// TODO: Do stuff
	while (client->state == STARTED)
	{
	}

	pthread_exit(NULL);
}

#include "stream.h"
#include "sock.h"
#include "barrier.h"
#include <cstddef>
#include <tr1/cstdint>
#include <cstdio>
#include <string>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <limits.h>


static char shared_buffer[BUFFER_SIZE];


Client::Client(Barrier& barr, const char* host, uint16_t rem_port, uint16_t loc_port)
	: state(INIT), barrier(barr), hostname(host), remote_port(rem_port), local_port(loc_port),
	  buf(NULL), buflen(BUFFER_SIZE), ival(0), _active(false)
{
	// Initialize synchronization primitives
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&start_signal, NULL);
	pthread_cond_init(&stop_signal, NULL);

	// Allocate buffer memory
	if (!set_chunk_size(BUFFER_SIZE))
		throw "Out of memory";


	// Create thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024);
	//pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	// Try to create thread
	pthread_mutex_lock(&mutex);
	if (pthread_create(&id, &attr, (void* (*)(void*)) &dispatch, (void*) this) == 0)
	{
		// Hold until the created thread reaches a certain execution point
		pthread_cond_wait(&stop_signal, &mutex);

		// Mark thread as started
		state = STARTED;
	}
	else
	{
		// Failed to create thread
		// TODO: Better error handling
		//throw "Failed to create thread";
		state = STOPPED;
	}
	pthread_mutex_unlock(&mutex);

	// Clean up thread attributes
	pthread_attr_destroy(&attr);
}



Client::~Client()
{
	// Stop thread
	stop();

	// Destroy synchronization primitives
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&start_signal);
	pthread_cond_destroy(&stop_signal);

	// Free buffer memory
//	if (buf != NULL)
//		delete[] buf;
}



bool Client::set_chunk_size(size_t bytes)
{
	bool success = false;

	if (bytes > BUFFER_SIZE)
		return false;

	pthread_mutex_lock(&mutex);
	if (state == STARTED || state == INIT)
	{
		if (buf == NULL)
			buf = shared_buffer;
		buflen = bytes;
		success = true;
//		char* temp = new char[bytes];
//		if (temp != NULL)
//		{
//			if (buf != NULL)
//				delete[] buf;
//			buf = temp;
//			buflen = bytes;
//			success = true;
//
//			for (unsigned i = 0; i < bytes; ++i)
//				buf[i] = 'A';
//		}
	}
	pthread_mutex_unlock(&mutex);

	return success;
}



bool Client::set_interval(unsigned ms)
{
	bool success = false;

	pthread_mutex_lock(&mutex);
	if (state == STARTED)
	{
		ival = ms;
		success = true;
	}
	pthread_mutex_unlock(&mutex);

	return success;
}



void Client::start()
{
	pthread_mutex_lock(&mutex);
	if (state == STARTED)
	{
		// Mark the thread as running
		state = RUNNING;

		// Signal to the waiting thread that it's good to go
		pthread_cond_signal(&start_signal);
	}
	pthread_mutex_unlock(&mutex);
}



void Client::stop()
{
	pthread_mutex_lock(&mutex);
	if (state != STOPPED)
	{
		// The client hasn't called start(), signal to the waiting thread that
		// it should start and stop immediatly
		if (state != RUNNING)
		{
			state = STOPPED;
			pthread_cond_signal(&start_signal);
			pthread_cond_wait(&stop_signal, &mutex);
		}

		// Wait for the thread to finish up
		state = STOPPED;
		// TODO: FIXME: This will lead to a race condition
		pthread_join(id, NULL);
	}
	pthread_mutex_unlock(&mutex);
}



void Client::run()
{
	// Create socket descriptor
	int sfd = -1;

	if (local_port != 0)
		sfd = Sock::create(hostname, remote_port, local_port);
	else
		sfd = Sock::create(hostname, remote_port);

	if (sfd == -1)
		throw "Can't connect";

	// Create sock instance and get connection information
	Sock sock(sfd);
	if (sfd != sock.raw() || !sock.alive())
		throw "Something is wrong";

	std::string host;
	uint16_t port;
	sock.peer(host);
	sock.peer(port);

	fprintf(stdout, "%s:%u Connection established (%d)\n", host.c_str(), port, sock.raw());
	fflush(stdout);

	// Synchronize with other clients by waiting on the barrier
	barrier.wait();

	// Create temporary variables
	timespec timeout = {0, 1000000L};
	ssize_t remaining, sent;
	uint64_t interval;
	bool full = false;

	// Write loop
	while (state == RUNNING && sock.alive())
	{
		// Sleep for an interval
		interval = ival;
		while (state == RUNNING && interval--)
			nanosleep(&timeout, NULL);

		// Send a chunk of data
		remaining = buflen;
		sent = 0;
		while (state == RUNNING && sock.alive() && remaining > 0)
		{
			sent = send(sfd, buf, buflen, MSG_DONTWAIT);

			if (sent == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
				break;
			else if (sent == -1 && errno == EPIPE)
				goto break_out;
			else if (sent == -1)
				goto break_out;

			// Check if send buffer is full
			if (ival > 0 && !full && sent == 0)
			{
				full = true;
				fprintf(stdout, "%s:%u Send buffer full (%d)\n", host.c_str(), port, sfd);
				fflush(stdout);
			}
			else if (full && sent > 0)
			{
				full = false;
				fprintf(stdout, "%s:%u Send buffer cleared (%d)\n", host.c_str(), port, sfd);
				fflush(stdout);
			}

			remaining -= sent;
		}
	}

break_out:
	fprintf(stdout, "%s:%u Disconnecting (%d)\n", host.c_str(), port, sfd);
	fflush(stdout);
	return;
}



void Client::dispatch(Client* client)
{
	pthread_mutex_lock(&client->mutex);

	// Notify parent thread that we have reached the execution point
	pthread_cond_signal(&client->stop_signal);

	// Wait until further notice
	pthread_cond_wait(&client->start_signal, &client->mutex);

	// Check if we are still good to go
	client->_active = client->state == RUNNING;

	pthread_mutex_unlock(&client->mutex);

	// Do the thread action
	try
	{
		if (client->_active)
			client->run();
	}
	catch (const char* exception)
	{
		client->_active = false;
		fprintf(stderr, "Couldn't connect to %s:%u: %s\n", client->hostname, client->remote_port, exception);
		client->barrier.wait();
	}

	client->_active = false;
	pthread_cond_signal(&client->stop_signal);
	pthread_exit(NULL);
}



bool Client::active()
{
	return _active;
}

#include <pthread.h>
#include <time.h>
#include "barrier.h"


/*
 * Initialize the barrier.
 */
Barrier::Barrier(int threads) :
	num_threads(threads),
	num_waiting(threads)
{
	pthread_cond_init(&waiting_queue, NULL);
	pthread_mutex_init(&lock, NULL);
}


/* 
 * Destroy the barrier.
 */
Barrier::~Barrier()
{
	pthread_cond_broadcast(&waiting_queue);
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&waiting_queue);
}


/*
 * Synchronize threads.
 */
void Barrier::wait()
{
	pthread_mutex_lock(&lock);
	if (num_threads == 1) {
		// Only one thread, artificially wait for 750 ms
		// FIXME: This is just an ugly hack to prevent the race condition for servers
		timespec timeout = {0, 750000000L};
		pthread_yield();
		nanosleep(&timeout, NULL);
	}

	if (--num_waiting > 0) {
		// Wait until all threads reach this point
		pthread_cond_wait(&waiting_queue, &lock);
	} else {
		// We are the last thread, wake everybody up and continue
		num_waiting = num_threads;
		pthread_cond_broadcast(&waiting_queue);
	}
	pthread_mutex_unlock(&lock);
}

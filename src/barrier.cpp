#include <pthread.h>
#include "barrier.h"


/*
 * Initialize the barrier
 */
Barrier::Barrier(int threads) :
	num_threads(threads),
	num_waiting(threads)
{
	pthread_cond_init(&waiting_queue, NULL);
	pthread_mutex_init(&lock, NULL);
}


/* 
 * Destroy the barrier
 */
Barrier::~Barrier()
{
	pthread_cond_broadcast(&waiting_queue);
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&waiting_queue);
}


/*
 * Synchronize threads
 */
void Barrier::wait()
{
	pthread_mutex_lock(&lock);
	if (--num_waiting > 0) {
		pthread_cond_wait(&waiting_queue, &lock);
	} else {
		num_waiting = num_threads;
		pthread_cond_broadcast(&waiting_queue);
	}
	pthread_mutex_unlock(&lock);
}

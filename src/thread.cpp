#include <pthread.h>
#include <cstddef>
#include "thread.h"


/*
 * Initialize a thread object
 */
Thread::Thread() :
	state(INIT)
{
	// Initialize synchronization primitives
	pthread_attr_init(&attr);
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&start_signal, NULL);
	pthread_cond_init(&stop_signal, NULL);

	pthread_mutex_lock(&mutex);

	// Initialize some thread attributes
	pthread_attr_setstacksize(&attr, 16000);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	// Try to create a thread
	if (pthread_create(&id, &attr, Thread::dispatch, (void*) this) == 0) {
		// Hold until the created thread reaches a certain execution point
		pthread_cond_wait(&stop_signal, &mutex);

		// Mark the thread as started
		state = STARTED;
	} else {

		// Mark the thread as failed
		state = STOPPED;
		// TODO: Add error handling if thread creation failed
		//pthread_mutex_unlock(&mutex);
		//throw "Failed to create thread";
	}
	pthread_mutex_unlock(&mutex);
}


/*
 * Destroy a thread object
 */
Thread::~Thread()
{
	// Stop thread by calling Thread::stop (not any derived version!)
	stop(); 

	// Destroy synchronization primitives
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&start_signal);
	pthread_cond_destroy(&stop_signal);
}


/*
 * Start the thread
 */
void Thread::start()
{
	pthread_mutex_lock(&mutex);
	if (state == STARTED) {
		// Mark the thread as good to go
		state = RUNNING;

		// Signal to the waiting thread that it's good to go
		pthread_cond_signal(&start_signal);
	}
	pthread_mutex_unlock(&mutex);
}


void Thread::stop()
{
	pthread_mutex_lock(&mutex);
	if (state != STOPPED) {

		// The client hasn't called start(), signal to the waiting thread that
		// it should start and stop immediatly.
		if (state != RUNNING) {
			state = STOPPED;
			pthread_cond_signal(&start_signal);
			pthread_cond_wait(&stop_signal, &mutex);
		}

		// Wait for the thread to finish up, we assume that the derived class'
		// stop() implementation has been called, and that run() behaves properly.
		state = STOPPED;
		pthread_join(id, NULL);
	}
	pthread_mutex_unlock(&mutex);
}


/*
 * Invoke run on a derived class
 */
void* Thread::dispatch(void* object)
{
	Thread* thread = static_cast<Thread*>(object);

	pthread_mutex_lock(&thread->mutex);
	// Notify parent thread that we have been created
	pthread_cond_signal(&thread->stop_signal);

	// Wait until further notice
	pthread_cond_wait(&thread->start_signal, &thread->mutex);

	// Check if we are still good to go
	bool run = thread->state == RUNNING;
	pthread_mutex_unlock(&thread->mutex);

	// Call the run() method of the derived class
	if (run) {
		thread->run();
	} 

	pthread_cond_signal(&thread->stop_signal);
	pthread_exit(NULL);
}

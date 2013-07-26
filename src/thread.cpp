#include <pthread.h>
#include <cstddef>
#include <cstdio>
#include "thread.h"



Thread::Thread() :
	state(INIT)
{
	pthread_attr_init(&attr);
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&start_signal, NULL);
	pthread_cond_init(&stop_signal, NULL);

	pthread_mutex_lock(&mutex);
	pthread_attr_setstacksize(&attr, 16000);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	if (pthread_create(&id, &attr, Thread::dispatch, (void*) this) == 0) {
		pthread_cond_wait(&stop_signal, &mutex);
		state = STARTED;
	} else {
		state = STOPPED;
		pthread_mutex_unlock(&mutex);
		throw "Failed to create thread";
	}
	pthread_mutex_unlock(&mutex);
}


Thread::~Thread()
{
	stop();

	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&start_signal);
	pthread_cond_destroy(&stop_signal);
}


void Thread::start()
{
	pthread_mutex_lock(&mutex);
	if (state == STARTED) {
		state = RUNNING;
		pthread_cond_signal(&start_signal);
	}
	pthread_mutex_unlock(&mutex);
}


void Thread::stop()
{
	pthread_mutex_lock(&mutex);
	if (state != STOPPED) {

		if (state != RUNNING) {
			state = STOPPED;
			pthread_cond_signal(&start_signal);
		}

		pthread_cond_wait(&stop_signal, &mutex);
		state = STOPPED;
	}
	pthread_mutex_unlock(&mutex);
}


void* Thread::dispatch(void* object)
{
	Thread* thread = static_cast<Thread*>(object);

	pthread_mutex_lock(&thread->mutex);
	pthread_cond_signal(&thread->stop_signal);
	pthread_cond_wait(&thread->start_signal, &thread->mutex);

	bool run = thread->state == RUNNING;

	pthread_mutex_unlock(&thread->mutex);

	if (run) {
		thread->run();
	}

	pthread_mutex_lock(&thread->mutex);
	pthread_cond_signal(&thread->stop_signal);
	pthread_mutex_unlock(&thread->mutex);

	pthread_exit(NULL);
}

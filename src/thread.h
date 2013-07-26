#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>


/* 
 * A generic base class for threads 
 *
 * Offers a simple to use interface to POSIX threads.
 * Please read the comments of this class carefully.
 */
class Thread
{
	public:
		explicit Thread(void);
		virtual ~Thread(void);
		
		/*
		 * Start the thread
		 */
		virtual void start(void);

		/* 
		 * Stop the thread
		 */
		virtual void stop(void);

		/*
		 * Do thread action
		 */
		virtual void run(void) = 0;

	private:
		enum { INIT, STARTED, RUNNING, STOPPED } state;
		pthread_t id;
		pthread_attr_t attr;
		pthread_mutex_t mutex;
		pthread_cond_t start_signal;
		pthread_cond_t stop_signal;

		static void* dispatch(void*);

		Thread& operator=(const Thread& other);
		Thread(const Thread& other);
};

#endif

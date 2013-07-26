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
		 *
		 * This method must be called from a derived class if overridden
		 */
		virtual void start(void);

		/*
		 * Stop the thread
		 *
		 * This method *must* be overridden, and it must be called as the
		 * very last thing of a derived class. If start() is called from
		 * the client code, then this method *must* also be called, otherwise
		 * bad things will happen.
		 */
		virtual void stop(void);

		/*
		 * Do thread action
		 *
		 * This method must be implemented by a derived class.
		 * It should not return, until stop() is called.
		 * When stop() is called on the derived class, this method *must*
		 * return, otherwise bad things will happen.
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

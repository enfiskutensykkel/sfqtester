#ifndef __BARRIER_H__
#define __BARRIER_H__

#include <pthread.h>


/* 
 * A generic thread barrier implemented as a monitor
 */
class Barrier
{
	public:
		explicit Barrier(int thread_count);
		~Barrier(void);
		
		/* 
		 * Synchronize threads
		 * 
		 * All threads will hold until every thread has called wait before 
		 * they continue from this point.
		 */
		void wait(void);

	private:
		int num_threads;
		int num_waiting;
		pthread_mutex_t lock;
		pthread_cond_t waiting_queue;

		/* Make the barrier object non-copyable */
		Barrier& operator=(const Barrier& other);
		Barrier(const Barrier& other);
};

#endif

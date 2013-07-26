#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>


class Thread
{
	public:
		explicit Thread(void);
		virtual ~Thread(void);
		
		virtual void start(void);
		virtual void stop(void);
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

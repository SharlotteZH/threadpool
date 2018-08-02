/**
* threadpool.c
*
* This file will contain your implementation of a threadpool.
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <queue>
#include "threadpool.h"

class work_t{
public:
	work_t(dispatch_fn	r, void *a): routine(r), arg(a){}
	dispatch_fn		routine;
	void   			*arg;
};


// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers

typedef struct _threadpool_st {
	// you should fill in this structure with whatever you need

	pthread_t* 					pwthread; /*pthread array*/

	pthread_cond_t    			pCondFull;
	pthread_cond_t    			pCondEmpty;
	pthread_mutex_t				pMutex;/*indicate the pool is empty*/

	int 						shutdown;/*1 indicate the pool will shutdown*/
	int 						nThread;/*indicate the numbers of threads in the pool */
	std::queue<work_t>		    *wQueue;	/*worker thread*/
} _threadpool;


static void cleanup(_threadpool *pool)
{
	delete pool->wQueue;
	if (pool->pwthread) {
		free (pool->pwthread);
		pool->pwthread = NULL;
	}
}

static int init_threadpool(_threadpool *pool, int num_threads_in_pool)
{
	pool->shutdown = 0;
	pool->nThread = 0;
	pool->pwthread = NULL;

	pool->wQueue = new std::queue<work_t>;
	if (!pool->wQueue) return -1;

	if ( pthread_mutex_init(&(pool->pMutex), NULL) ) {
		printf("init pool->pMutex error!\n");
		return -1;
	}
	if ( pthread_cond_init(&(pool->pCondFull), NULL) ) {
		printf("init pool->pCondFull error!\n");
		return -1;
	}
	if ( pthread_cond_init(&(pool->pCondEmpty), NULL) ) {
		printf("init pool->pCondEmpty error!\n");
		return -1;
	}

	return 0;
}

void* workproc(threadpool p) {
	_threadpool * pool = (_threadpool *) p;

	for(;;) {
		pthread_mutex_lock(&(pool->pMutex));
		while ( pool->wQueue->empty() ) {	//if the size is 0 then wait.
			if (pool->shutdown) {
				pool->nThread--;
				if (!pool->nThread) pthread_cond_signal(&(pool->pCondFull));
				pthread_mutex_unlock(&(pool->pMutex));
				pthread_exit(NULL);
			}
			pthread_cond_wait(&(pool->pCondEmpty), &(pool->pMutex));
		}

		work_t& cur = pool->wQueue->front(); pool->wQueue->pop();
		pthread_mutex_unlock(&(pool->pMutex));

		(cur.routine) (cur.arg);   //actually do work.

		pthread_mutex_lock(&(pool->pMutex));
		if ( ! (pool->wQueue->size() == pool->nThread) && ! pool->shutdown)
			pthread_cond_signal(&(pool->pCondFull));
		pthread_mutex_unlock(&(pool->pMutex));
	}
}

int dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
			 void *arg) {
	_threadpool *pool = (_threadpool *) from_me;
	int ret = 0;

	pthread_mutex_lock(&(pool->pMutex));

	while (pool->wQueue->size() == pool->nThread) {
		pthread_cond_wait(&(pool->pCondFull), &(pool->pMutex));
	}

	if(pool->shutdown) { //Just incase someone is trying to queue more
		//should work structs.
		printf("queue more: should unlock pool->pMutexEmpty?\n");
		ret = -1;
        goto exit;
	}

	pool->wQueue->emplace(dispatch_to_here, arg);

	if(!pool->wQueue->empty()) {
		pthread_cond_signal(&(pool->pCondEmpty));  //I am not empty.
	}

exit:
	pthread_mutex_unlock(&(pool->pMutex));  //unlock the queue.
    return ret;
}


threadpool create_threadpool(int num_threads_in_pool) {
	_threadpool *pool;
	int i;
	// sanity check the argument
	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
		return NULL;

	pool = (_threadpool *) malloc(sizeof(_threadpool));
	if (pool == NULL) {
		printf("Out of memory creating a new threadpool!\n");
		return NULL;
	}

	// add your code here to initialize the newly created threadpool
	if ( init_threadpool(pool, num_threads_in_pool) < 0 ) {
        printf("init pool error!\n");
        cleanup(pool);
        return NULL;
    }

	pool->pwthread = (pthread_t*)malloc(num_threads_in_pool*sizeof(pthread_t));
	if (!pool->pwthread) {
		printf("init pool->pwthread error!\n");
		cleanup(pool);
        return NULL;
	}

	for (i = 0;i < num_threads_in_pool;i++) {
		if(pthread_create(&(pool->pwthread[i]), NULL, workproc, pool)) {
			printf("Thread initiation error!\n");
			return NULL;
		}
		pool->nThread++;
	}
	return (threadpool) pool;
}

void destroy_threadpool(threadpool destroyme) {
	_threadpool *pool = (_threadpool *) destroyme;

	// add your code here to kill a threadpool
	pthread_mutex_lock(&(pool->pMutex));
	pool->shutdown = 1;  //allow shutdown
	pthread_cond_broadcast(&(pool->pCondEmpty));//allow code to return NULL;

	while (pool->nThread > 0)
		pthread_cond_wait(&(pool->pCondFull), &(pool->pMutex));

	pthread_mutex_unlock(&(pool->pMutex));
	cleanup (pool);
	free (pool);
}

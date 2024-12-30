#ifndef PROMISE_H
#define PROMISE_H
#include<pthread.h>
#include<stdio.h>
#include"./dynamicPtr.h"
#include"./queue.h"

typedef struct
{
	Queue* q;
	dynamicPtr parameter;
	pthread_t ptid;
} Promise;

typedef dynamicPtr (*first_func)();
typedef dynamicPtr (*next_func)(dynamicPtr);

void* promiseThread(void* arg)
{
	pthread_detach(pthread_self());

	Promise* p = (Promise*)arg;

	while(!isEmpty(p->q))
	{
		dynamicPtr rear = dequeue(p->q);
		next_func nextFunc = (next_func)(rear.data);
		p->parameter = (*nextFunc)(p->parameter);
	}

	free(p->q);
	free(p);

	return 0;
}

Promise* createPromise(next_func func, dynamicPtr args)
{
	Promise* p = malloc(sizeof(Promise));
	p->q = createQueue(256);
	enqueue(p->q, dp((void*)(func)));
	p->parameter = args;

	return p;
}

void then(Promise* p, dynamicPtr func(dynamicPtr))
{
	enqueue(p->q, dp((void*)(func)));
}

void invoke(Promise* p)
{
	pthread_t ptid;
	pthread_create(&ptid, NULL, promiseThread, (void*)p);
	p->ptid = ptid;
}


#endif
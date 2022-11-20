/*************************************************************************
	> File Name: thrd_pool.h
	> Author: 
	> Mail: 
	> Created Time: Fri 18 Nov 2022 04:44:54 AM PST
 ************************************************************************/

#ifndef _THRD_POOL_H
#define _THRD_POOL_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

typedef void (*handler_pt) (void *);

typedef struct task_t {
	handler_pt func;
	void *arg;
} task_t;

typedef struct task_queue_t {
	uint32_t head;
	uint32_t tail;
	uint32_t count;
	task_t *queue;
} task_queue_t;

typedef struct thread_pool_t {
	// 互斥访问任务队列
	pthread_mutex_t mutex;
	// 生产者消费者同步
	pthread_cond_t condition;
	
	task_queue_t task_queue;
	int queue_size;
	
	int closed;
	
	pthread_t *threads;
	int started;
	int thrd_count;

}thread_pool_t ;

thread_pool_t *thread_pool_create(int thrd_count, int queue_size);

int thread_pool_post(thread_pool_t *pool, handler_pt func, void *arg);

int thread_pool_destroy(thread_pool_t *pool);

int wait_all_done(thread_pool_t *pool);

#endif

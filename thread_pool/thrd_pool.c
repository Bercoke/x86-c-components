#include "thrd_pool.h"

static void *thread_worker(void *pool);
static void thread_pool_free(thread_pool_t *thrd_pool);

thread_pool_t *
thread_pool_create(int thrd_count, int queue_size) {
    if(thrd_count <= 0 || queue_size <= 0) {
        return NULL;
    }

    thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if(pool == NULL) {
        return NULL;
    }
    pool->started = pool->closed = 0;

    pool->task_queue.queue = (task_t *)malloc(sizeof(task_t) * queue_size);
    if(pool->task_queue.queue == NULL) {
        thread_pool_free(pool);
        return NULL;
    }
    pool->queue_size = queue_size;
    pool->task_queue.head = pool->task_queue.tail = 0;
    pool->task_queue.count = 0;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t));
    if(pool->threads == NULL) {
        thread_pool_free(pool);
        return NULL;
    }
    pool->thrd_count = 0;
    for(int i = 0; i < thrd_count; ++i) {
        if(pthread_create(&pool->threads[i], NULL, thread_worker, (void *)pool) != 0) {
            thread_pool_free(pool);
            return NULL;
        }
        pool->thrd_count++;
        pool->started++;
    }

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->condition, NULL);

    return pool;
}

// 向任务队列中添加任务, 主程序为生产者
int 
thread_pool_post(thread_pool_t *pool, handler_pt func, void *arg) {
    if(pool == NULL || func == NULL) {
        return -1;
    }

    task_queue_t *task_queue = &(pool->task_queue);

    if(pthread_mutex_lock(&(pool->mutex)) != 0) {
        return -2;
    }

    if(pool->closed) {
        pthread_mutex_unlock(&(pool->mutex));
        return -3;
    }

    // 任务队列满了
    if(task_queue->count == pool->queue_size) {
        pthread_mutex_unlock(&(pool->mutex));
        return -4;
    }

    task_queue->queue[task_queue->tail].func = func;
    task_queue->queue[task_queue->tail].arg = arg;
    task_queue->tail = (task_queue->tail + 1) % pool->queue_size;
    task_queue->count++;

    if(pthread_cond_signal(&(pool->condition)) != 0) {
        pthread_mutex_unlock(&(pool->mutex));
        return -5;
    }
    pthread_mutex_unlock(&(pool->mutex));
    return 0;
}

static void 
thread_pool_free(thread_pool_t *pool) {
    if (pool == NULL || pool->started > 0) {
        return;
    }

    if (pool->threads) {
        free(pool->threads);
        pool->threads = NULL;

        pthread_mutex_lock(&(pool->mutex));
        pthread_mutex_destroy(&pool->mutex);
        pthread_cond_destroy(&pool->condition);
    }

    if (pool->task_queue.queue) {
        free(pool->task_queue.queue);
        pool->task_queue.queue = NULL;
    }
    free(pool);
}

// 等待所有线程都工作完
int 
wait_all_done(thread_pool_t *pool) {
    int ret = 0;
    for(int i = 0; i < pool->thrd_count; ++i) {
        if(pthread_join(pool->threads[i], NULL) != 0) {
            ret = 1;
        }
    }
    return ret;
}

int thread_pool_destroy(thread_pool_t *pool) {
    if(pool == NULL) {
        return -1;
    }

    if(pthread_mutex_lock(&(pool->mutex)) != 0) {
        return -2;
    }

    if(pool->closed) {
        thread_pool_free(pool);
        return -3;
    }

    pool->closed = 1;

    if(pthread_cond_broadcast(&(pool->condition)) != 0 || 
            pthread_mutex_unlock(&(pool->mutex)) != 0) {
        thread_pool_free(pool);
        return -4;
    }


    wait_all_done(pool);
    thread_pool_free(pool);
    return 0;
}

// 消费者, worker线程
static void *
thread_worker(void *thrd_pool) {
    
    thread_pool_t *pool = (thread_pool_t *)thrd_pool;
    task_queue_t *que;
    task_t task;

    for(;;) {
        pthread_mutex_lock(&(pool->mutex));
        que = &(pool->task_queue);

        while(que->count == 0 && pool->closed == 0) {
            pthread_cond_wait(&(pool->condition), &(pool->mutex));
        }

        if(pool->closed == 1) break;
        task = que->queue[que->head];
        que->head = (que->head + 1) % pool->queue_size;
        que->count--;
        pthread_mutex_unlock(&(pool->mutex));
        
        (*(task.func))(task.arg);
    }

    pool->started--;
    pthread_mutex_unlock(&(pool->mutex));
    pthread_exit(NULL);
    return NULL;
}

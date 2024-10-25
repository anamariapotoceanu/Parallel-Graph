// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"
pthread_cond_t condition;

/* Create a task that would be executed by a thread. */
os_task_t *create_task(void (*action)(void *), void *arg, void (*destroy_arg)(void *))
{
	os_task_t *t;

	t = malloc(sizeof(*t));
	DIE(t == NULL, "malloc");

	t->action = action;		// the function
	t->argument = arg;		// arguments for the function
	t->destroy_arg = destroy_arg;	// destroy argument function

	return t;
}

/* Destroy task. */
void destroy_task(os_task_t *t)
{
	if (t->destroy_arg != NULL)
		t->destroy_arg(t->argument);
	free(t);
}

/* Put a new task to threadpool task queue. */
void enqueue_task(os_threadpool_t *tp, os_task_t *t)
{
	assert(tp != NULL);
	assert(t != NULL);

	pthread_mutex_lock(&tp->mutex);
	list_add_tail(&tp->head, &t->list);
	tp->total_tasks++;
	pthread_cond_signal(&tp->cond);
	pthread_mutex_unlock(&tp->mutex);
}

/*
 * Get a task from threadpool task queue.
 * Block if no task is available.
 * Return NULL if work is complete, i.e. no task will become available,
 * i.e. all threads are going to block.
 */
os_task_t *dequeue_task(os_threadpool_t *tp)
{
	os_task_t *t = NULL;

	pthread_mutex_lock(&tp->mutex);

	for (; list_empty(&tp->head) && !tp->stop; )
		pthread_cond_wait(&tp->cond, &tp->mutex);

	if (!list_empty(&tp->head)) {
		t = list_entry(tp->head.next, os_task_t, list);
		list_del(&t->list);
		tp->total_tasks--;
		pthread_cond_signal(&tp->cond);
	}

	pthread_mutex_unlock(&tp->mutex);

	return t;
}

/* Loop function for threads */
static void *thread_loop_function(void *arg)
{
	os_threadpool_t *tp = (os_threadpool_t *) arg;

	while (1) {
		os_task_t *t;

		t = dequeue_task(tp);
		if (t == NULL)
			break;
		t->action(t->argument);
		destroy_task(t);
	}

	return NULL;
}

/* Wait completion of all threads. This is to be called by the main thread. */
void wait_for_completion(os_threadpool_t *tp)
{
	pthread_mutex_lock(&tp->mutex);
	tp->stop = 1;
	pthread_cond_broadcast(&tp->cond);

	for (; tp->total_tasks > 0; )
		pthread_cond_wait(&tp->cond, &tp->mutex);

	pthread_mutex_unlock(&tp->mutex);

	/* Join all worker threads. */
	for (unsigned int i = 0; i < tp->num_threads; i++)
		pthread_join(tp->threads[i], NULL);
}

/* Create a new threadpool. */
os_threadpool_t *create_threadpool(unsigned int num_threads)
{
	os_threadpool_t *tp;

	tp = calloc(1, sizeof(*tp));

	DIE(tp == NULL, "calloc");

	list_init(&tp->head);
	int rc_init;

	rc_init = pthread_mutex_init(&tp->mutex, NULL);

	DIE(rc_init < 0, "pthread_mutex_init");
	int rc_cond;

	rc_cond = pthread_cond_init(&tp->cond, NULL);

	DIE(rc_cond < 0, "pthread_cond_init");

	tp->total_tasks = 0;
	tp->stop = 0;
	tp->num_threads = num_threads;
	tp->threads = calloc(num_threads, sizeof(*tp->threads));
	DIE(tp->threads == NULL, "calloc");

	for (unsigned int i = 0; i < num_threads; ++i) {
		int rc;

		rc = pthread_create(&tp->threads[i], NULL, &thread_loop_function, (void *)tp);

		if (rc < 0) {
			free(tp->threads);
			free(tp);
			DIE(rc < 0, "pthread_create");
			return NULL;
		}
	}

	return tp;
}

/* Destroy a threadpool. Assume all threads have been joined. */
void destroy_threadpool(os_threadpool_t *tp)
{
	os_list_node_t *n, *p;

	list_for_each_safe(n, p, &tp->head) {
		pthread_mutex_lock(&tp->mutex);
		list_del(n);
		os_task_t *task = list_entry(n, os_task_t, list);

		destroy_task(task);
		pthread_mutex_unlock(&tp->mutex);
	}
	int rc_mutex;

	rc_mutex = pthread_mutex_destroy(&tp->mutex);

	DIE(rc_mutex < 0, "pthread_mutex_destroy");

	int rc_cond;

	rc_cond = pthread_cond_destroy(&tp->cond);

	DIE(rc_cond < 0, "pthread_cond_destroy");

	free(tp->threads);
	free(tp);
}

// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
static pthread_mutex_t *node_mutexes;
pthread_mutex_t summ;

typedef struct {
	unsigned int node_index;
} graph_task_arg_t;

static void process_node(void *arg)
{
	graph_task_arg_t *task_arg;

	task_arg = (graph_task_arg_t *)arg;
	unsigned int idx;
	int info;

	idx = task_arg->node_index;

	pthread_mutex_lock(&summ);
	pthread_mutex_lock(&node_mutexes[idx]);
	info = graph->nodes[idx]->info;
	sum = sum + info;
	pthread_mutex_unlock(&node_mutexes[idx]);
	pthread_mutex_unlock(&summ);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	node_mutexes = malloc(graph->num_nodes * sizeof(pthread_mutex_t));
	DIE(node_mutexes == NULL, "malloc");

	unsigned int i, nr;

	nr = graph->num_nodes;

	for (i = 0; i < nr; i++) {
		int rc;

		rc = pthread_mutex_init(&node_mutexes[i], NULL);

		DIE(rc < 0, "pthread_mutex_init");
	}
	int rc_mutex;

	rc_mutex = pthread_mutex_init(&summ, NULL);

	DIE(rc_mutex < 0, "pthread_mutex_init");

	tp = create_threadpool(NUM_THREADS);

	for (i = 0; i < nr; i++) {
		graph_task_arg_t *task_arg;

		task_arg = malloc(sizeof(graph_task_arg_t));

		DIE(task_arg == NULL, "malloc");

		task_arg->node_index = i;

		os_task_t *task;

		task = create_task(process_node, (void *)task_arg, free);

		enqueue_task(tp, task);
	}

	wait_for_completion(tp);
	destroy_threadpool(tp);
	int rc_destroy;

	rc_destroy = pthread_mutex_destroy(&summ);

	DIE(rc_destroy < 0, "pthread_mutex_destroy");

	printf("%d", sum);

	for (i = 0; i < nr; i++) {
		int rc_destroy;

		rc_destroy = pthread_mutex_destroy(&node_mutexes[i]);

		DIE(rc_destroy < 0, "pthread_mutex_destroy");
	}

	free(node_mutexes);

	return 0;
}

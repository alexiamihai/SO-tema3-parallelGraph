// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "log/log.h"
#include "os_graph.h"
#include "os_threadpool.h"
#include "utils.h"

#define NUM_THREADS 4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t graph_mutex;
pthread_mutex_t graph_mutex2;
/* TODO: Define graph task argument. */
typedef struct {
	unsigned int idx;
} graph_task_arg_t;
static void process_neighbours(graph_task_arg_t *arg);

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */

	os_node_t *node = graph->nodes[idx];

	pthread_mutex_lock(&graph_mutex2);
	graph->visited[idx] = DONE;
	os_task_t *task;

	graph_task_arg_t *arg = malloc(sizeof(*arg));

	arg->idx = node->id;
	task = create_task((void (*)(void *))process_neighbours, arg, free);
	enqueue_task(tp, task);
	pthread_mutex_unlock(&graph_mutex2);
}

static void process_neighbours(graph_task_arg_t *arg)
{
	os_node_t *node = graph->nodes[arg->idx];

	int local_sum = node->info;

	pthread_mutex_lock(&graph_mutex);
	sum += local_sum;
	pthread_mutex_unlock(&graph_mutex);
	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		pthread_mutex_lock(&graph_mutex);
		if (graph->visited[node->neighbours[i]] == NOT_VISITED)
			process_node(node->neighbours[i]);
		pthread_mutex_unlock(&graph_mutex);
	}
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
	/* TODO: Initialize graph synchronization mechanisms. */
	pthread_mutex_init(&graph_mutex, NULL);
	pthread_mutex_init(&graph_mutex2, NULL);
	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);
	pthread_mutex_destroy(&graph_mutex2);
	pthread_mutex_destroy(&graph_mutex);
	printf("%d", sum);
	return 0;
}

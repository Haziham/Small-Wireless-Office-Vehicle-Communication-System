/*
 *  queue.c
 *  Version: 28 May 2022
 *
 *  This file provides the implementation for Queue using a linked-list.
 *
 */ 


#include <stdlib.h>
#include "node.h"
#include "queue.h"

struct queue_int {
	node first;
};

/*
 * 'Constructor' for queue
 */
void queue_init(queue *q)
{
	*q = (queue)malloc(sizeof(struct queue_int));
	(*q)->first = NULL;
}

/*
 * Check for emptiness
 * Return true if queue is empty, false otherwise
 */
char queue_isEmpty(queue q)
{
	return (q->first == NULL);
}

/*
 * Find front item in queue
 * Return value at front of queue q
 */
void *queue_get(queue q)
{
	return node_getData(q->first);
}

/*
 * Remove first item from queue q
 */
void queue_remove(queue q)
{
	node tofree;
	tofree = q->first;
	q->first = node_getNext(q->first);
	node_deleteData(tofree);
	free(tofree);
}

/*
 * Add item to rear of queue q
 * Param o value to be added to rear of queue
 */
void queue_add(queue q, void *o)
{
	node n;
	node_init(&n, o);

	node c;
	c = q->first;
	if (queue_isEmpty(q))
	{
		q->first = n;
	}
	else
	{
		while (node_getNext(c) != NULL)
		{
			c = node_getNext(c);
		}
		node_setNext(c, n);
	}
}
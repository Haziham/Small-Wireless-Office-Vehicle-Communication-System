/*
 *  queue.h
 *  Version: 28 May 2022
 * 
 *	This file holds the specification of the queue.
 *
 */


#ifndef QUEUE_H_
#define QUEUE_H_

struct queue_int;
typedef struct queue_int *queue;

void queue_init(queue *q);
char queue_isEmpty(queue q);
void queue_add(queue q, void *i);
void *queue_get(queue q);
void queue_remove(queue q);

#endif
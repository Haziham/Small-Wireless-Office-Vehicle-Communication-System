/*
 *  node.h
 *  Version: 28 May 2022
 *
 *	This file holds the specification of the linked-list node.
 *
 */


#ifndef NODE_H_
#define NODE_H_

struct node_int;
typedef struct node_int *node;

void node_init(node *vp, void *o);
void node_setData(node v, void *o);
void node_setNext(node v, node n);
void *node_getData(node v);
node node_getNext(node v);
void node_deleteData(node n);

#endif 
/*
 *  node.c
 *  Version: 28 May 2022
 *
 *  This file provides the implementation for Queue using a linked-list.
 *
 */


#include "node.h"
#include <stdlib.h>

struct node_int {
	void *data;
	node next;
};

/*
 * 'Constructor' for node
 */
void node_init(node *np, void* o)
{
	*np = (node)malloc(sizeof(struct node_int));
	(*np)->data = o;
	(*np)->next = NULL;
}

/*
 * Getter for data
 * Return data field
 */
void *node_getData(node n)
{
	return n->data;
}

/*
 * Getter for next
 * Return next field
 */
node node_getNext(node n)
{
	return n->next;
}

/*
 * Setter for data
 * Param o value to be placed into the node's data field
 */
void node_setData(node n,void *o)
{
	n->data = o;
}

/*
 * Setter for next
 * Param n node to be placed into the node's next field
 */
void node_setNext(node v, node n)
{
	v->next = n;
}

/*
 * Delete node data
 * Param n node's data field to be freed
 */
void node_deleteData(node n)
{
	free(n->data);
}
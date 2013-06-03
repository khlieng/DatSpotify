#include <stdlib.h>
#include <string.h>

#include "fifo.h"

fifo_t * fifo_new() {
	fifo_t * fifo = (fifo_t *)malloc(sizeof(fifo_t));
	memset(fifo, 0, sizeof(fifo_t));

	return fifo;
}

void fifo_free(fifo_t * q) {
	fifo_flush(q);
	free(q);
}

void fifo_flush(fifo_t * q) {
	if (q->len > 0) {
		//void * data;
		while(fifo_pop(q)) { 
			//free(data);
		}
	}
}

void fifo_push(fifo_t * q, void * data) {
	node_fifo_t * node = (node_fifo_t *)malloc(sizeof(node_fifo_t));
	memset(node, 0, sizeof(node_fifo_t));
	node->data = data;

	if (q->tail) {
		q->head->next = node;
		q->head = node;
		q->len++;		
	}
	else {
		q->head = node;
		q->tail = node;
		q->len = 1;
	}
}

void * fifo_pop(fifo_t * q) {
	if (q->tail) {
		node_fifo_t * tail = q->tail;
		void * data = tail->data;

		q->tail = tail->next;
		q->len--;
		free(tail);

		return data;
	}
	return NULL;
}
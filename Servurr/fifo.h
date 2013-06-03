#ifndef FIFO_H
#define FIFO_H

typedef struct node_fifo node_fifo_t;
typedef struct fifo fifo_t;

struct node_fifo {
	void * data;
	node_fifo_t * next;
};

struct fifo {
	node_fifo_t * head;
	node_fifo_t * tail;
	size_t len;
};

fifo_t * fifo_new();
void fifo_free(fifo_t * q);
void fifo_flush(fifo_t * q);
void fifo_push(fifo_t * q, void * data);
void * fifo_pop(fifo_t * q);

#endif
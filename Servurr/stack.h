#ifndef STACK_H
#define STACK_H

typedef struct node_stack node_stack_t;
typedef struct stack stack_t;

struct node_stack {
	void * data;
	node_stack_t * prev;
};

struct stack {
	node_stack_t * head;
	size_t len;
};

stack_t * stack_new();
void stack_free(stack_t * q);
void stack_push(stack_t * q, void * data);
void * stack_pop(stack_t * q);

#endif
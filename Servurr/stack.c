#include <stdlib.h>
#include <string.h>

#include "stack.h"

stack_t * stack_new() {
	stack_t * stack = (stack_t *)malloc(sizeof(stack_t));
	memset(stack, 0, sizeof(stack_t));

	return stack;
}

void stack_free(stack_t * s) {
	if (s->len > 0) {
		while(stack_pop(s)) { }
	}
	free(s);
}

void stack_push(stack_t * s, void * data) {
	node_stack_t * node = (node_stack_t *)malloc(sizeof(node_stack_t));
	memset(node, 0, sizeof(node_stack_t));
	node->data = data;

	if (s->head) {
		node->prev = s->head;
		s->head = node;
		s->len++;		
	}
	else {
		s->head = node;
		s->len = 1;
	}
}

void * stack_pop(stack_t * s) {
	if (s->head) {
		node_stack_t * head = s->head;
		void * data = head->data;

		s->head = head->prev;
		s->len--;
		free(head);

		return data;
	}
	return NULL;
}
#pragma once

#include <stdbool.h>

typedef struct list_head {
	struct list_head *next, *prev;
} list_head_t;

#define LIST_HEAD_INIT(head_ptr) do { \
    (head_ptr)->next = (head_ptr); (head_ptr)->prev = (head_ptr) \
} while (0)

static inline void list_add(list_head_t *head,
                            list_head_t *new)
{
	list_head_t *next = head->next;
	next->prev = new;
	new->next = next;
	new->prev = head;
	head->next = new;
}

/// assuming a linked list starting at `head`, it appends to the end of it
static inline void list_append(list_head_t *head,
                               list_head_t *new)
{
	list_head_t *tail = head->prev;
	tail->next = new;
	new->prev = tail;
	new->next = head;
	head->prev = new;
}

static inline void list_remove(const list_head_t *const node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

static inline bool list_empty(const list_head_t *const head)
{
	return head->next == head;
}
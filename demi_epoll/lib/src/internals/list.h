#pragma once

#include <stdbool.h>
#include <stddef.h>

#define container_of(_list_ptr, _type, _member) ((_type*)((void*)(_list_ptr) - offsetof(_type, _member)))

typedef struct list_elem {
	struct list_elem *next, *prev;
} list_elem_t;

#define LIST_HEAD_INIT(head_ptr) do { \
    (head_ptr)->next = (head_ptr); (head_ptr)->prev = (head_ptr); \
} while (0)

static inline void list_add(list_elem_t *head,
                            list_elem_t *new)
{
	list_elem_t *next = head->next;
	next->prev = new;
	new->next = next;
	new->prev = head;
	head->next = new;
}

static inline void list_add_to_head(list_elem_t **head, list_elem_t *new)
{
	if (*head)
		list_add(*head, new);
	else {
		LIST_HEAD_INIT(new);
		*head = new;
	}
}

/// assuming a linked list starting at `head`, it appends to the end of it
static inline void list_append(list_elem_t *head,
                               list_elem_t *new)
{
	list_elem_t *tail = head->prev;
	tail->next = new;
	new->prev = tail;
	new->next = head;
	head->prev = new;
}

static inline void list_remove(const list_elem_t *const node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

static inline bool list_is_empty(const list_elem_t *const head)
{
	return head->next == head;
}

static inline bool list_contains_elem(list_elem_t *head, list_elem_t *elem)
{
	return (head == elem) || !list_is_empty(elem);
}


#include <acc/list.h>

struct node {
	struct node * previous;
	struct node * next;
	void * data;
};

struct list {
	struct node * head;
	struct node * last;
	size_t length;
};

struct list * new_list(void * init[], int count)
{
	struct list * result;
	struct node * prev, * head;
	int i;

	result = malloc(sizeof(struct list));

	if (!init) {
		result->head = NULL;
		result->last = NULL;
		result->length = 0;
		return result;
	}

	prev = NULL;
	head = NULL;
	for (i = 0; i < count; ++i) {
		struct node * n;
		n = malloc(sizeof(struct node));
		n->data = init[i];
		n->next = NULL;
		n->previous = prev;
		if (prev) {
			prev->next = n;
		}
		if (!head) {
			head = n;
		}
		prev = n;
	}

	result->head = head;
	result->last = prev;
	result->length = count;
	return result;
}

struct list * clone_list(struct list * l)
{
	struct list * result;
	struct node * prev, * head;
	void * item, * it;

	result = new_list(NULL, 0);
	prev = NULL;
	head = NULL;
	it = list_iterator(l);
	while (item = iterator_next(&it)) {
		struct node * n;
		n = malloc(sizeof(struct node));
		n->data = item;
		n->next = NULL;
		n->previous = prev;
		if (prev) {
			prev->next = n;
		}
		if (!head) {
			head = n;
		}
		prev = n;
	}

	result->head = head;
	result->last = prev;
	result->length = list_length(l);
	return result;
}

void delete_list(struct list * l, void (*destr)(void *))
{
	struct node * n, * next;
	next = l->head;
	while (next) {
		n = next;
		if (destr) {
			destr(n->data);
		}
		next = n->next;
		free(n);
	}
	free(l);
}

void * list_iterator(struct list * l)
{
	return l->head;
}

void * iterator_next(void ** it)
{
	struct node * n;
	void * item;
	n = *it;
	if (!n) {
		return NULL;
	}
	item = n->data;
	*it = n->next;
	return item;
}

void * get_list_item(struct list * l, int idx)
{
	struct node * n;
	n = l->head;
	while (idx-- > 0) {
		n = n->next;
	}
	return n->data;
}

void set_list_item(struct list * l, int idx, void * data)
{
	struct node * n;
	n = l->head;
	while (idx-- > 0) {
		n = n->next;
	}
	n->data = data;
}

void list_push_back(struct list * l, void * data)
{
	struct node * n;
	n = malloc(sizeof(struct node));
	n->next = NULL;
	n->previous = l->last;
	n->data = data;
	if (l->last) {
		l->last->next = n;
	}
	l->last = n;
	++l->length;
}

void * list_pop_back(struct list * l)
{
	void * data;
	struct node * nl;
	data = l->last->data;

	nl = l->last->previous;
	if (!nl) {
		l->last = NULL;
		l->head = NULL;
		return data;
	}
	nl->next = NULL;
	free(l->last);
	l->last = nl;
	return data;
}

void list_push_front(struct list * l, void * data)
{
	struct node * n;
	n = malloc(sizeof(struct node));
	n->previous = NULL;
	n->next = l->head;
	n->data = data;
	l->head = n;
	++l->length;
}

void * list_pop_front(struct list * l)
{
	void * data;
	struct node * nh;
	data = l->head->data;

	nh = l->head->next;
	if (!nh) {
		l->last = NULL;
		l->head = NULL;
		return data;
	}
	nh->previous = NULL;
	free(l->head);
	l->head = nh;
	return data;
}

void * list_head(struct list * l)
{
	return l->head->data;
}

void * list_last(struct list * l)
{
	return l->last->data;
}

size_t list_length(struct list * l)
{
	return l->length;
}

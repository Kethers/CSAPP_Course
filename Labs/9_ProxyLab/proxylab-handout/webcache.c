#include "webcache.h"

void init_webcache()
{
	readcnt = 0;
	init_webcache_sem();
	init_webcache_container(&g_wc_container);
}

static void init_webcache_sem()
{
	Sem_init(&mutex, 0, 1);
	Sem_init(&w, 0, 1);
}

static void init_webcache_container(webcache_container_t* wcp)
{
	wcp->head = (webcache_t*)Malloc(sizeof(webcache_t));
    memset(wcp->head, 0, sizeof(webcache_t));
	wcp->tail = (webcache_t*)Malloc(sizeof(webcache_t));
    memset(wcp->tail, 0, sizeof(webcache_t));
	wcp->head->next = wcp->tail;
	wcp->tail->prev = wcp->head;
	wcp->remain_cache_size = MAX_CACHE_SIZE;
}

webcache_t* find_cache_node(webcache_container_t* wcp, char* filename)
{
	webcache_t* target_node = NULL;

	P(&mutex);
	readcnt++;
	if (readcnt == 1)
		P(&w);
	V(&mutex);

	for (webcache_t* i = wcp->head; i != wcp->tail; i = i->next)
	{
		if (i->filename && !strcmp(i->filename, filename))     // cache hit
		{
			target_node = i;
			break;
		}
	}
	
	P(&mutex);
	readcnt--;
	if (readcnt == 0)
		V(&w);
	V(&mutex);

	return target_node;
}

void move_node_to_first(webcache_container_t* wcp, webcache_t* node)
{
	P(&w);

	node = remove_node(wcp, node);
	if (node)
	{
		add_first(wcp, node);
	}

	V(&w);
}

static webcache_t* remove_node(webcache_container_t* wcp, webcache_t* node)
{
	for (webcache_t* i = wcp->head; i != wcp->tail; i = i->next)
	{
		if (i == node)
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
			wcp->remain_cache_size += node->content_len;
			return node;
		}
	}

	return NULL;
}

void remove_last_ts(webcache_container_t* wcp)
{
	P(&w);

	webcache_t *last = wcp->tail->prev;
	last->prev->next = last->next;
	last->next->prev = last->prev;
	free_cache_node(last);

	V(&w);
}

static void free_cache_node(webcache_t* node)
{
	node->next = node->prev = NULL;
	Free(node->filename);
	Free(node->rsp_hdr);
	Free(node->rsp_body);
	Free(node);
}

void add_first_ts(webcache_container_t* wcp, webcache_t* node)
{
	if (node == NULL)
		return;

	P(&w);
	add_first(wcp, node);
	V(&w);
}


static void add_first(webcache_container_t* wcp, webcache_t* node)
{
	if (node == NULL || node->content_len > MAX_OBJECT_SIZE)
		return;

	node->prev = wcp->head;
	node->next = wcp->head->next;
	node->next->prev = node;
	node->prev->next = node;
	wcp->remain_cache_size -= node->content_len;
}

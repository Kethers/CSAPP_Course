#pragma once

#include "csapp.h"
#include "util.h"

typedef struct webcache
{
	struct webcache *prev;
	struct webcache *next;
	char* filename;
	char* rsp_hdr;
	char* rsp_body;
	size_t content_len;
} webcache_t;

typedef struct webcache_container
{
	webcache_t *head, *tail;
	size_t remain_cache_size;
} webcache_container_t;

static int readcnt;
static sem_t mutex, w;

webcache_container_t g_wc_container;

void init_webcache();

static void init_webcache_sem();
static void init_webcache_container(webcache_container_t* wcp);

webcache_t* find_cache_node(webcache_container_t* wcp, char* filename);

void move_node_to_first(webcache_container_t* wcp, webcache_t* node);

static webcache_t* remove_node(webcache_container_t* wcp, webcache_t* node);

void remove_last_ts(webcache_container_t* wcp);

static void free_cache_node(webcache_t* node);

void add_first_ts(webcache_container_t* wcp, webcache_t* node);

static void add_first(webcache_container_t* wcp, webcache_t* node);

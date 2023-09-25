# Solving Proxy Lab Step by Step

### Errata of driver.sh

At line 301, originally is:

```shell
./nop-server.py ${nop_port} &> /dev/null &
```

supposed to be:

```shell
python3 ./nop-server.py ${nop_port} &> /dev/null &
```



## 1. Overall Design

Although in the handout write-up it’s suggested that we implement sequential web proxy, concurrency and caching one by one, I think it a better idea to write the concurrency and caching at the very beginning, since the textbook gives detailed implementation of most of our code patterns.

First step is to design data structure for concurrency and caching. Worker thread receive task from main thread and finish it, use producer-consumer pattern will be fine. Prethreading is also introduced in the textbook and code of these two patterns almost remain unchanged in final solution. For caching data structure, of course we need to use reader-writer pattern. The question is what do we need for caching web object and LRU eviction.

Think about the time we use cache web object to reply the client request:

1. how do we know whether the request resource is in cache or not? Obviously, we can use *filename* as the key to traverse through the cache.
2. If cache hit, to make up our HTTP response, we should get both header (actually this header is rsp line and rsp header, just for convenience since they’re in the same block in rsp) and body in the cache node.
3. (prev and) next ptr are needed for LRU eviction. What’s more, the total cache size is limited thus we need to record the size we used for caching this object.

A good beginning is the half done. Actually when we have listed above cautions, we are not far from the end since except the concurrency and caching most other part are just I/O stuff and string manipulations.



## 2. Data Structures

Code for producer-consumer pattern is identical to the textbook.

Code for reader-writer pattern is likely to the textbook except that we need to modify the critical section of reading and writing.

From previous discussion we know that cache should be implemented via double-linked list. Thus we use `webcache_container_t` as if the C++ container for double-linked list but much simpler:

```c
typedef struct webcache_container
{
	webcache_t *head, *tail;
	size_t remain_cache_size;
} webcache_container_t;
```

`remain_cache_size` is initialized as `MAX_CACHE_SIZE` and will be changed when manipulating the list.

The cache node is `webcache_t` and shown below:

```c
typedef struct webcache
{
	struct webcache *prev;
	struct webcache *next;
	char* filename;
	char* rsp_hdr;
	char* rsp_body;
	size_t content_len;
} webcache_t;
```

We implement the manipulating functions in `webcache.c` and use `static` keyword to limit the variables/functions caller scope within the file as if they are “private” (OOP).

The first node in the list is the one just accessed while the last node is the one haven’t be accessed for long.

So what functions do we need to exposed as “public”? Initialization of course. Remove last node for eviction. Add node at head when caching. Move cache node to head when accessing it (read/write).

Furthermore, to deal with edge-case conveniently, we allocate two node for both head and tail with no extra heap space within “class” node.

`add_first_ts`: *O(1)*

```c
void add_first_ts(webcache_container_t* wcp, webcache_t* node)
{
	if (node == NULL)
		return;

	P(&w);
	add_first(wcp, node);
	V(&w);
}
```

`remove_last_ts`: *O(1)*

```c
void remove_last_ts(webcache_container_t* wcp)
{
	P(&w);

	webcache_t *last = wcp->tail->prev;
	last->prev->next = last->next;
	last->next->prev = last->prev;
	free_cache_node(last);

	V(&w);
}
```

`move_node_to_first`: *O(n)*

```c
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
```

As we can see, we just use P&V semaphore operation to warp our critical section code. `remove_node` and `add_first` are not thread-safe, but calling them in above cases are thread-safe.

`find_cache_node`: *O(n)*

```c
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
```



## 3. Network

When forwarding the request from client, we need to remain the Host field or add it when not given, and remove Connection, Proxt-Connection and User-Agent fields then use our own.

Most code are easy if-else and string manipulations, just be careful.



## 4. Termination Prevention

According to hints of the writeup, we need to modify the behavior of “SIGPIPE” and x_error functions provided by csapp.c. Just block SIGPIPE and comment the `exit(0)` in x_error functions, or make your own error printing for better debugging.



## 5. Debugging

As writeup has said, we can use curl + tiny + proxy + vscode(gdb) for debugging. Set breakpoints and single-stepping through them and watch the values of different strings are expected or not.



## 6. Result

```shell
$ ./driver.sh 
*** Basic ***
Starting tiny on 30712
Starting proxy on 28013
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy
   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40

*** Concurrency ***
Starting tiny on port 25474
Starting proxy on port 33463
Starting the blocking NOP server on port 3167
Trying to fetch a file from the blocking nop-server
Fetching ./tiny/home.html into ./.noproxy directly from Tiny
Fetching ./tiny/home.html into ./.proxy using the proxy
Checking whether the proxy fetch succeeded
Success: Was able to fetch tiny/home.html from the proxy.
Killing tiny, proxy, and nop-server
concurrencyScore: 15/15

*** Cache ***
Starting tiny on port 16650
Starting proxy on port 29743
Fetching ./tiny/tiny.c into ./.proxy using the proxy
Fetching ./tiny/home.html into ./.proxy using the proxy
Fetching ./tiny/csapp.c into ./.proxy using the proxy
Killing tiny
Fetching a cached copy of ./tiny/home.html into ./.noproxy
Success: Was able to fetch tiny/home.html from the cache.
Killing proxy
cacheScore: 15/15

totalScore: 70/70
```


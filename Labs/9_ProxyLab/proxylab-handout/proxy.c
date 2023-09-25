#include "csapp.h"
#include "sbuf.h"
#include "webcache.h"
#include "util.h"

/* Recommended max cache and object sizes */
#define NTHREADS 4
#define SBUFSIZE 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void read_requesthdrs(rio_t *rp, char* hdrbuf);
int parse_uri(char *uri, char* hostname, char* port, char *filename);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void* worker_thread(void* vargp);
void parse_rsphdr(char* hdrbuf, char* rsp_code, size_t* content_len);
void do_client(int fd, char* hostname, char* port, char* filename, char* hdrbuf);
void do_server(int fd, char* hdrbuf, char* bodybuf, size_t content_len);

sbuf_t sbuf;

int main(int argc, char **argv) 
{
	int i, listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;

	/* Check command line args */
	if (argc != 2) 
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	sigset_t mask_one, prev_one;
	Sigemptyset(&mask_one);
	Sigaddset(&mask_one, SIGPIPE);
	Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

	listenfd = Open_listenfd(argv[1]);

	sbuf_init(&sbuf, SBUFSIZE);
	init_webcache();
	for (i = 0; i < NTHREADS; i++)
	{
		Pthread_create(&tid, NULL, worker_thread, NULL);
	}
	
	while (1) 
	{
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
		sbuf_insert(&sbuf, connfd);

		Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
	}

	sbuf_deinit(&sbuf);
}

void* worker_thread(void* vargp)
{
	Pthread_detach(pthread_self());
	while (1)
	{
		int connfd = sbuf_remove(&sbuf);
		doit(connfd);
		Close(connfd);
	}
}

void parse_rsphdr(char* hdrbuf, char* rsp_code, size_t* content_len)
{
	char version[MAXLINE];
	sscanf(hdrbuf, "%s %s", version, rsp_code);
	if (!strcmp(rsp_code, "200"))
	{
		char* clen = strstr(hdrbuf, "Content-length: ");
		if (clen != NULL)
		{
			char* lineend = strstr(clen, "\r\n");
			char* num_start = clen + strlen("Content-length: ");
			size_t num_len = lineend - num_start;
			strncpy(version, num_start, num_len);
			version[num_len] = '\0';
			*content_len = atoi(version);
		}
	}
}

void do_client(int fd, char* hostname, char* port, char* filename, char* hdrbuf)
{
	char buf[MAXLINE], linebuf[MAXLINE], rsp_code[MAXLINE];
	size_t content_len;
	rio_t rio;

	int clientfd = Open_clientfd(hostname, port);

	strcpy(buf, "GET ");
	strcat(buf, filename);
	strcat(buf, " HTTP/1.0\r\n");
	
	strcat(buf, hdrbuf);
	linebuf[0] = '\0';
	buf[strlen(buf)-2] = '\0';
	
	if (strstr(buf, "Host:") == NULL)
	{
		strcat(buf, hostname);
		strcat(buf, "\r\n");
	}
	strcat(buf, user_agent_hdr);
	strcat(buf, "Connection: close\r\n");
	strcat(buf, "Proxy-Connection: close\r\n");
	strcat(buf, "\r\n\0");

	Rio_writen(clientfd, buf, strlen(buf));

	buf[0] = '\0';
	Rio_readinitb(&rio, clientfd);
	do
	{
		Rio_readlineb(&rio, linebuf, MAXLINE);
		strcat(buf, linebuf);
	} while(strcmp(linebuf, "\r\n"));

	parse_rsphdr(buf, rsp_code, &content_len);
	if (!strcmp(rsp_code, "200"))	// 200 OK
	{
		if (content_len <= MAX_OBJECT_SIZE)		// cache the object
		{
			webcache_t* node = find_cache_node(&g_wc_container, filename);
			if (node)	// already in cache
			{
				move_node_to_first(&g_wc_container, node);
			}
			else
			{
				node = (webcache_t*)Malloc(sizeof(webcache_t));
				node->content_len = content_len;

				node->filename = (char*)Malloc(strlen(filename) + 1);
				strcpy(node->filename, filename);

				node->rsp_hdr = (char*)Malloc(strlen(buf) + 1);
				strcpy(node->rsp_hdr, buf);

				node->rsp_body = (char*)Malloc(content_len);
				Rio_readnb(&rio, node->rsp_body, content_len);

				while (g_wc_container.remain_cache_size < content_len)
				{
					remove_last_ts(&g_wc_container);
				}
				add_first_ts(&g_wc_container, node);
			}

			Rio_writen(fd, node->rsp_hdr, strlen(node->rsp_hdr));
			Rio_writen(fd, node->rsp_body, content_len);
		}
		else
		{
			char* bodybuf = (char*)Malloc(content_len);
			Rio_readnb(&rio, bodybuf, content_len);
			do_server(fd, buf, bodybuf, content_len);
			Free(bodybuf);
		}
	}
	else
	{
		Rio_writen(fd, buf, strlen(buf));
	}
}

void do_server(int fd, char* hdrbuf, char* bodybuf, size_t content_len)
{
	printf("tid: %ld, content_len: %ld\n%s%s", pthread_self(), content_len, hdrbuf, bodybuf);
	Rio_writen(fd, hdrbuf, strlen(hdrbuf));
	Rio_writen(fd, bodybuf, content_len);
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) 
{
	int is_static;
	// struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char hostname[MAXLINE], port[MAXLINE], filename[MAXLINE];
	char hdrbuf[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	if (!Rio_readlineb(&rio, buf, MAXLINE))
		return;
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET"))
	{
		clienterror(fd, method, "501", "Not Implemented",
					"proxy does not implement this method");
		return;
	}
	/* Parse URI from GET request */
	is_static = parse_uri(uri, hostname, port, filename);
	read_requesthdrs(&rio, hdrbuf);

	webcache_t* node = find_cache_node(&g_wc_container, filename);
	if (node)
	{
		do_server(fd, node->rsp_hdr, node->rsp_body, node->content_len);
	}
	else
	{
		do_client(fd, hostname, port, filename, hdrbuf);
	}
}



/*
 * read_requesthdrs - read HTTP request headers
 */
void read_requesthdrs(rio_t *rp, char* hdrbuf) 
{
	char buf[MAXLINE];

	hdrbuf[0] = '\0';

	do
	{
		Rio_readlineb(rp, buf, MAXLINE);
		if (!(strstr(buf, "Connection:") 
			|| strstr(buf, "Proxy-Connection:")
			|| strstr(buf, "User-Agent:")))
		{
			strcat(hdrbuf, buf);
		}
		// printf("%s", buf);
	} while (strcmp(buf, "\r\n"));
	return;
}



/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse_uri(char *uri, char* hostname, char* port, char *filename) 
{
	char *ptr;
	char *http_begin = strstr(uri, "http://");;
	char *file_uri = uri;
	char *port_colon_begin = strstr(http_begin == NULL ? uri : uri + strlen("http://"), ":");
	char *file_slash_begin = NULL, *hostbegin = NULL;

	if (port_colon_begin)
	{
		file_slash_begin = strstr(port_colon_begin, "/");
		if (file_slash_begin != NULL)
			strncpy(port, port_colon_begin + 1, file_slash_begin - port_colon_begin - 1);
		else
			strcpy(port, port_colon_begin + 1);
	}

	if (http_begin != NULL)
	{
		file_slash_begin = strstr(uri + strlen("http://"), "/");
		hostbegin = http_begin + strlen("http://");
	}
	else
	{
		file_slash_begin = strstr(uri, "/");
		hostbegin = uri;
	}
	if (file_slash_begin != NULL && port_colon_begin == NULL)
	{
		strncpy(hostname, hostbegin, file_slash_begin - hostbegin);
	}
	else if (port_colon_begin != NULL)
	{
		strncpy(hostname, hostbegin, port_colon_begin - hostbegin);
	}
	else
	{
		strcpy(hostname, hostbegin);
	}
	file_uri = file_slash_begin;

	if (!strstr(file_uri, "cgi-bin"))
	{  /* Static content */ //line:netp:parseuri:isstatic
		strcpy(filename, "");                           //line:netp:parseuri:beginconvert1
		strcat(filename, file_uri);                           //line:netp:parseuri:endconvert1
		if (file_uri[strlen(file_uri)-1] == '/')                   //line:netp:parseuri:slashcheck
			strcat(filename, "home.html");               //line:netp:parseuri:appenddefault
		return 1;
	}
	else
	{  /* Dynamic content */                        //line:netp:parseuri:isdynamic
		strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
		strcat(filename, file_uri);                           //line:netp:parseuri:endconvert2
		return 0;
	}
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
	char buf[MAXLINE];

	/* Print the HTTP response headers */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n\r\n");
	Rio_writen(fd, buf, strlen(buf));

	/* Print the HTTP response body */
	sprintf(buf, "<html><title>Tiny Error</title>");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
	Rio_writen(fd, buf, strlen(buf));
}

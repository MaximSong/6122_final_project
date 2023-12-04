#include <iostream>
#include <sstream>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LRU_NUMBER 9999
#define CACHE_COUNT 10

// request header format
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *endof_hdr = "\r\n";
static const char *connection_key = "Connection";
static const char *user_agent_key= "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

typedef struct {
    char cache_object[MAX_OBJECT_SIZE];
    char cache_url[MAXLINE];
    int LRU;
    bool is_empty;

    int read_cnt;
    sem_t cache_mtx;
    sem_t read_cnt_mtx;
} Cache_block;

typedef struct {
    Cache_block cache_objects[CACHE_COUNT];
    int cache_num;
} Cache;

Cache cache;

void cache_init()
{
    cache.cache_num = 0;
    for (int i = 0; i < CACHE_COUNT; i++) 
    {
        cache.cache_objects[i].LRU = 0;
        cache.cache_objects[i].is_empty = true;
        Sem_init(&cache.cache_objects[i].cache_mtx, 0, 1);
        Sem_init(&cache.cache_objects[i].read_cnt_mtx, 0, 1);
        cache.cache_objects[i].read_cnt = 0;
    }
}

void cache_reader_pre(int idx)
{
    P(&cache.cache_objects[idx].read_cnt_mtx);
    cache.cache_objects[idx].read_cnt++;

    if (cache.cache_objects[idx].read_cnt == 1)
    {
        P(&cache.cache_objects[idx].cache_mtx);
    }

    V(&cache.cache_objects[idx].read_cnt_mtx);
}

void cache_reader_after(int idx)
{
    P(&cache.cache_objects[idx].read_cnt_mtx);
    cache.cache_objects[idx].read_cnt--;
    
    if (cache.cache_objects[idx].read_cnt == 0)
    {
        V(&cache.cache_objects[idx].cache_mtx);
    }

    V(&cache.cache_objects[idx].read_cnt_mtx);
}

void cache_write_pre(int idx)
{
    P(&cache.cache_objects[idx].cache_mtx);
}

void cache_write_after(int idx)
{
    V(&cache.cache_objects[idx].cache_mtx);
}

// check whether url is in the cache or not
int find_cache(char *url)
{
    int i;
    for (i = 0; i < CACHE_COUNT; i++)
    {
        cache_reader_pre(i);
        if ((!cache.cache_objects[i].is_empty) && (strcmp(url, cache.cache_objects[i].cache_url) == 0))
            break;
        cache_reader_after(i);
    }

    if (i == CACHE_COUNT) return -1;
    return i;
}

// find the empty cache_object or which cache_object should be removed
int cache_remove()
{
    int min_LRU = LRU_NUMBER;
    int min_index = 0;

    for(int i = 0; i < CACHE_COUNT; i++)
    {
        cache_reader_pre(i);
        if (cache.cache_objects[i].is_empty)
        {
            // if the cache block is empty, then choose it
            min_index = i;
            cache_reader_after(i);
            break;
        }
        if (cache.cache_objects[i].LRU < min_LRU)
        {    
            // if cache is not empty, then choose the min LRU
            min_index = i;
            cache_reader_after(i);
            continue;
        }
        cache_reader_after(i);
    }

    return min_index;
}

// update the LRU number
void cache_LRU(int idx)
{
    int i;
    for (i = 0; i < idx; i++)
    {
        cache_write_pre(i);
        if (!cache.cache_objects[i].is_empty)
        {
            cache.cache_objects[i].LRU--;
        }
        cache_write_after(i);
    }

    i++;

    for(; i < CACHE_COUNT; i++)
    {
        cache_write_pre(i);
        if(!cache.cache_objects[i].is_empty && i != idx)
        {
            cache.cache_objects[i].LRU--;
        }
        cache_write_after(i);
    }
}

// cache the uri and content
void cache_uri(char *uri, char *buf)
{
    int i = cache_remove();

    cache_write_pre(i);

    strcpy(cache.cache_objects[i].cache_object, buf);
    strcpy(cache.cache_objects[i].cache_url, uri);
    cache.cache_objects[i].is_empty = false;
    cache.cache_objects[i].LRU = LRU_NUMBER;
    cache_LRU(i);
    cache_write_after(i);
}

void build_http_header(char *http_header, char *hostname, char *path, int port, rio_t *client_rio)
{
    char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];
    // request line
    sprintf(request_hdr, requestlint_hdr_format, path);
    // get other request header for client rio and change it
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0)
    {
        if (strcmp(buf, endof_hdr) == 0) break;

        // host
        if (!strncasecmp(buf, host_key, strlen(host_key)))
        {
            strcpy(host_hdr, buf);
            continue;
        }

        if (!strncasecmp(buf, connection_key, strlen(connection_key))
                && !strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key))
                && !strncasecmp(buf, user_agent_key, strlen(user_agent_key)))
        {
            strcat(other_hdr,buf);
        }
    }

    if (strlen(host_hdr) == 0)
    {
        sprintf(host_hdr, host_hdr_format, hostname);
    }

    sprintf(http_header, "%s%s%s%s%s%s%s",
            request_hdr,
            host_hdr,
            conn_hdr,
            prox_hdr,
            user_agent_hdr,
            other_hdr,
            endof_hdr);
    return ;
}

// Connect to the end server
inline int connect_endServer(char *hostname, int port, char *http_header)
{
    char portStr[100];
    sprintf(portStr, "%d", port); // int to string
    return Open_clientfd(hostname, portStr);
}

// parse the uri to get hostname, file path, and port
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
    // default port
    *port = 80;
    // find the first position of "//"
    char* pos = strstr(uri, "//");
    
    pos = pos != NULL ? pos + 2 : uri;

    char* pos2 = strstr(pos, ":");

    if (pos2 != NULL)
    {
        *pos2 = '\0';
        sscanf(pos, "%s", hostname);
        sscanf(pos2 + 1, "%d%s", port, path);
    }
    else
    {
        pos2 = strstr(pos, "/");
        if (pos2 != NULL)
        {
            *pos2 = '\0';
            sscanf(pos, "%s", hostname);
            *pos2 = '/';
            sscanf(pos2, "%s", path);
        }
        else
        {
            sscanf(pos, "%s", hostname);
        }
    }
    return;
}

// handle client transactions
void doit(int connfd)
{
    // end server socket
    int end_serverfd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

     // store the http_header
    char endserver_http_header [MAXLINE];

    // store the request line arguments
    char hostname[MAXLINE], path[MAXLINE];
    int port;

    // rio is client's rio, server_rio is endserver's rio
    rio_t rio, server_rio;
    // Initiate the buf
    Rio_readinitb(&rio, connfd);
    // Read data from the buf
    Rio_readlineb(&rio, buf, MAXLINE);

    std::istringstream iss(buf);
    iss >> method >> uri >> version;
 
    // We only process GET Method
    if (strcasecmp(method, "GET"))
    {
        std::cout << "Proxy does not implement the method" << std::endl;
        return;
    }

    // store the original url
    char url_store[100];
    strcpy(url_store, uri);

    // check whether the uri is cached
    int cache_idx;
    if ((cache_idx = find_cache(url_store)) != -1)
    {
        cache_reader_pre(cache_idx);
        Rio_writen(connfd, cache.cache_objects[cache_idx].cache_object, strlen(cache.cache_objects[cache_idx].cache_object));
        cache_reader_after(cache_idx);
        return;
    }

    // parse the uri to get hostname, file path, and port
    parse_uri(uri, hostname, path, &port);
    // build the http header which will send to the end server
    build_http_header(endserver_http_header,hostname,path,port,&rio);
    // connect to the end server
    end_serverfd = connect_endServer(hostname, port, endserver_http_header);
    
    if (end_serverfd < 0)
    {
        std::cout << "Connection failed" << std::endl;
        return;
    }

    // init the read buffer
    Rio_readinitb(&server_rio, end_serverfd);

    // write the http header to endserver
    Rio_writen(end_serverfd, endserver_http_header, strlen(endserver_http_header));

    char cache_buffer[MAX_OBJECT_SIZE];
    int buffer_size = 0;

    // receive message from end server and send to the client
    size_t n;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0)
    {
        buffer_size += n;
        if (buffer_size < MAX_OBJECT_SIZE) {
            strcat(cache_buffer, buf);
        }
        std::cout << "proxy received " << n << " bytes, then send" << std::endl;
        Rio_writen(connfd, buf, n);
    }

    Close(end_serverfd);

    // store the url
    if (buffer_size < MAX_OBJECT_SIZE)
    {
        cache_uri(url_store, cache_buffer);
    }
}

void *thread_routine(void *vargp)
{
    intptr_t connfd = reinterpret_cast<intptr_t>(vargp); 
    Pthread_detach(pthread_self());
    doit(connfd);
    Close(connfd);
}

int main(int argc, char **argv)
{
    // listening socket 
    int listenfd; 
    // client connection socket
    int connfd; 

    // address len
    socklen_t clientlen;
    //available hostnames
    char hostname[MAXLINE]; 
    // available ports
    char port[MAXLINE]; 

    pthread_t tid;

    struct sockaddr_storage clientaddr;

    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <port>\n";
        exit(EXIT_FAILURE);
    }

    cache_init();
    Signal(SIGPIPE, SIG_IGN);
    listenfd = Open_listenfd(argv[1]);

    while (true)
    {
        clientlen = sizeof(clientaddr);
        // accept the pending connection on the listening socket
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        std::cout << "Accepted connection from (" << hostname << " " << port << ").\n";
        // create a new thread to handle the client transaction
        Pthread_create(&tid, NULL, thread_routine, reinterpret_cast<void *>(connfd));
    }

}
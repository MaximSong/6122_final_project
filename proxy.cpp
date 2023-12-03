#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include "csapp.h"
using namespace std;
/* Max cache size and object size */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
/* request header format, we only deal with GET request from the client */
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

void build_http_header(char *http_header, char *hostname, char *path, int port, rio_t *client_rio)
{
    char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];
    /* request line */
    sprintf(request_hdr, requestlint_hdr_format, path);
    /* get other request header for client rio and change it */
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0)
    {
        if (strcmp(buf, endof_hdr) == 0) break; /* EOF */

        if (!strncasecmp(buf, host_key, strlen(host_key))) /* Host: */
        {
            strcpy(host_hdr, buf);
            continue;
        }

        if (!strncasecmp(buf, connection_key, strlen(connection_key))
                && !strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key))
                && !strncasecmp(buf, user_agent_key, strlen(user_agent_key)))
        {
            strcat(other_hdr, buf);
        }
    }
    if (strlen(host_hdr) == 0)
    {
        sprintf(host_hdr, host_hdr_format, hostname);
    }

    sprintf(http_header,"%s%s%s%s%s%s%s",
            request_hdr,
            host_hdr,
            conn_hdr,
            prox_hdr,
            user_agent_hdr,
            other_hdr,
            endof_hdr);

    return ;
}

/* Connect to the end server */
inline int connect_endServer(char *hostname, int port, char *http_header)
{
    char portStr[100];
    // int to string
    sprintf(portStr, "%d", port);
    return Open_clientfd(hostname, portStr);
}

/* parse the uri to get hostname, file path, port */
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
    *port = 80; // default port
    char* pos = strstr(uri, "//"); // find the first position of "//"
    
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
        if(pos2 != NULL)
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

/* handle client transactions */
void doit(int connfd)
{
    int end_serverfd; // end server socket
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char endserver_http_header[MAXLINE]; // store the http_header
    /* store the request line arguments */
    char hostname[MAXLINE], path[MAXLINE];
    int port;

    /* rio is client's rio, server_rio is endserver's rio */
    rio_t rio, server_rio;
    Rio_readinitb(&rio, connfd); // Initiate the buf
    Rio_readlineb(&rio, buf, MAXLINE); //  Read data from the buf

    istringstream iss(buf);
    iss >> method >> uri >> version;
    // We only process GET Method
    if (strcasecmp(method, "GET"))
    {
        cout << "Proxy does not implement the method" << endl;
        return;
    }
    /* parse the uri to get hostname, file path, port */
    parse_uri(uri, hostname, path, &port);
    /* build the http header which will send to the end server */
    build_http_header(endserver_http_header, hostname, path, port, &rio);
    /* connect to the end server */
    end_serverfd = connect_endServer(hostname, port, endserver_http_header);

    if (end_serverfd < 0)
    {
        cout << "Connection failed" << endl;
        return;
    }
    Rio_readinitb(&server_rio, end_serverfd); //init the read buffer
    /* write the http header to endserver */
    Rio_writen(end_serverfd, endserver_http_header, strlen(endserver_http_header));

    /* receive message from end server and send to the client */
    size_t n;

    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0)
    {
        cout << "proxy received " << n << " bytes, then send" << endl;
        Rio_writen(connfd, buf, n);
    }
    Close(end_serverfd);

}

int main(int argc, char **argv)
{
    int listenfd; // listening socket 
    int connfd; // client connection socket
    socklen_t clientlen; // address len
    char hostname[MAXLINE]; //available hostnames
    char port[MAXLINE]; // available ports

    /* generic sockaddr struct which is 28 Bytes. The same use as sockaddr */
    struct sockaddr_storage clientaddr;

    if (argc != 2)
    {
        cerr << "usage: " << argv[0] << " <port>\n";
        exit(EXIT_FAILURE);
    }

    listenfd = Open_listenfd(argv[1]);
    
    while (true)
    {
        clientlen = sizeof(clientaddr);
        /* accept the pending connection on the listening socket */
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        cout << "Accepted connection from (" << hostname << " " << port << ").\n";
        doit(connfd);
        Close(connfd);
    }
    return 0;
}

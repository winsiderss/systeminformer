#include <phdk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "tcp.h"

int growl_tcp_parse_hostname( const char *const server , int default_port , struct sockaddr_in *const sockaddr  );

void growl_tcp_write_raw( SOCKET sock, const unsigned char * data, const int data_length )
{
    send(sock, data, data_length, 0);
}

void growl_tcp_write( SOCKET sock , const char *const format , ... ) 
{
    int length;
    char *output;
    char *stop;

    va_list ap;

    va_start( ap , format );
    length = vsnprintf( NULL , 0 , format , ap );
    va_end(ap);

    va_start(ap,format);
    output = (char*)PhAllocateSafe(length+1);
    if (!output) {
        va_end(ap);
        return;
    }
    vsnprintf( output , length+1 , format , ap );
    va_end(ap);

    while ((stop = strstr(output, "\r\n"))) strcpy(stop, stop + 1);

    send( sock , output , length , 0 );
    send( sock , "\r\n" , 2 , 0 );

    PhFree(output);
}

char *growl_tcp_read(SOCKET sock) {
    const int growsize = 80;
    char c = 0;
    char* line = (char*) PhAllocateSafe(growsize);
    if (line) {
        int len = growsize, pos = 0;
        char* newline;
        while (line) {
            if (recv(sock, &c, 1, 0) <= 0) break;
            if (c == '\r') continue;
            if (c == '\n') break;
            line[pos++] = c;
            if (pos >= len) {
                len += growsize;
                newline = (char*) realloc(line, len);
                if (!newline) {
                    PhFree(line);
                    return NULL;
                }
                line = newline;
            }
        }
        line[pos] = 0;
    }
    return line;
}

SOCKET growl_tcp_open(const char* server) {
    SOCKET sock = -1;
#ifdef _WIN32
    char on;
#else
    int on;
#endif
    struct sockaddr_in serv_addr;

    if( growl_tcp_parse_hostname( server , 23053 , &serv_addr ) == -1 )
    {
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("create socket");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return -1;
    }

    on = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1) {
        perror("setsockopt");
        return -1;
    }

    return sock;
}

void growl_tcp_close(SOCKET sock) {
#ifdef _WIN32
    if (sock > 0) closesocket(sock);
#else
    if (sock > 0) close(sock);
#endif
}

int growl_tcp_parse_hostname( const char *const server , int default_port , struct sockaddr_in *const sockaddr )
{
    char *hostname = PhDuplicateAnsiStringZSafe((PSTR)server);
    char *port = strchr( hostname, ':' );
    struct hostent* host_ent;
    if( port != NULL )
    {
        *port = '\0';
        port++;
        default_port = atoi(port);
    }
    
    host_ent = gethostbyname(hostname);
    if( host_ent == NULL )
    {
        perror("gethostbyname");
        PhFree(hostname);
        return -1;
    }
    
    memset( sockaddr , 0 , sizeof(sockaddr) );
    sockaddr->sin_family = AF_INET;
    memcpy( &sockaddr->sin_addr , host_ent->h_addr , host_ent->h_length );
    sockaddr->sin_port = htons(default_port);
     
    PhFree(hostname);
    return 0;
}

int growl_tcp_datagram( const char *server , const unsigned char *data , const int data_length )
{
    struct sockaddr_in serv_addr;
    SOCKET sock = 0;

    if( growl_tcp_parse_hostname( server , 9887 , &serv_addr ) == -1 )
    {
        return -1;
    }
    
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if( sock == INVALID_SOCKET )
    {
        return -1;
    }
    
    if( sendto(sock, (char*)data , data_length , 0 , (struct sockaddr*)&serv_addr , sizeof(serv_addr) ) > 0 )
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

#ifndef _TCP_H_
#define _TCP_H_

#ifdef _MSC_VER
#define __attribute__(x)
#endif
void growl_tcp_write_raw( SOCKET sock, const unsigned char * data, const int data_length );
void growl_tcp_write( SOCKET sock , const char *const format , ... ) __attribute__ ((format (printf, 2, 3)));
char* growl_tcp_read(SOCKET sock);
SOCKET growl_tcp_open(const char* server);
void growl_tcp_close(SOCKET sock);
int growl_tcp_datagram( const char *server , const unsigned char *data , const int data_length );

#endif /* _TCP_H_ */

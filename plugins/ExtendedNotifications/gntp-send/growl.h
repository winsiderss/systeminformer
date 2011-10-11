#ifndef _GROWL_H_
#define _GROWL_H_

#ifdef __cplusplus
extern "C" {
#endif


int growl( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url );
int growl_tcp_notify( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const password, const char* const url, const char* const icon );
int growl_tcp_register( const char *const server , const char *const appname , const char **const notifications , const int notifications_count , const char *const password, const char *const icon );


int growl_udp( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url );
int growl_udp_notify( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const password );
int growl_udp_register( const char *const server , const char *const appname , const char **const notifications , const int notifications_count , const char *const password );


int growl_init(void);
void growl_shutdown(void);

extern char *gntp_send_license_text;

#ifdef __cplusplus
}
#endif


#endif /* _GROWL_H_ */

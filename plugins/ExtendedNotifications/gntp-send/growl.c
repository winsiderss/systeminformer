#include <phdk.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "tcp.h"
#include "growl.h"

static const char hex_table[] = "0123456789ABCDEF";
static char* string_to_hex_alloc(const char* str, int len) {
    int n, l;
    char* tmp = (char*)PhAllocateSafe(len * 2 + 1);
    if (tmp) {
        memset(tmp, 0, len * 2 + 1);
        for (l = 0, n = 0; l < len; l++) {
            tmp[n++] = hex_table[(str[l] & 0xF0) >> 4];
            tmp[n++] = hex_table[str[l] & 0x0F];
        }
        }
    return tmp;
}

int growl_init_ = 0;

int growl_init()
{
        if( growl_init_ == 0)
        {
                #ifdef _WIN32
                WSADATA wsaData;
                if( WSAStartup( MAKEWORD( 2 , 0 ) , &wsaData) != 0 )
                {
                        return -1;
                }
                #endif

                srand((unsigned int)time(NULL));
                growl_init_ = 1;
        }
        return 1;
}


void growl_shutdown()
{
        if( growl_init_ == 1 )
        {
                #ifdef _WIN32
                WSACleanup();
                #endif
        }
}


char* gen_salt_alloc(int count) {
    char* salt = (char*)PhAllocateSafe(count + 1);
    if (salt) {
        int n;
        for (n = 0; n < count; n++) salt[n] = (((int)rand()) % 255) + 1;
        salt[n] = 0;
    }
    return salt;
}

char* gen_password_hash_alloc(const char* password, const char* salt) {
    md5_context md5ctx;
    char md5tmp[20];
    char* md5digest;

    memset(md5tmp, 0, sizeof(md5tmp));
    md5_starts(&md5ctx);
    md5_update(&md5ctx, (uint8_t*)password, strlen(password));
    md5_update(&md5ctx, (uint8_t*)salt, strlen(salt));
    md5_finish(&md5ctx, (uint8_t*)md5tmp);

    md5_starts(&md5ctx);
    md5_update(&md5ctx, (uint8_t*)md5tmp, 16);
    md5_finish(&md5ctx, (uint8_t*)md5tmp);
    md5digest = string_to_hex_alloc(md5tmp, 16);

    return md5digest;
}

char *growl_generate_authheader_alloc(const char*const password)
{
    char* salt;
    char* salthash;
    char* keyhash;
    char* authheader = NULL;

    if (password) {
        salt = gen_salt_alloc(8);
        if (salt) {
            keyhash = gen_password_hash_alloc(password, salt);
            if (keyhash) {
                salthash = string_to_hex_alloc(salt, 8);
                if (salthash) {
                    authheader = (char*)PhAllocateSafe(strlen(keyhash) + strlen(salthash) + 7);
                    if (authheader) {
                        sprintf(authheader, " MD5:%s.%s", keyhash, salthash);
                    }
                    PhFree(salthash);
                }
                PhFree(keyhash);
            }
            PhFree(salt);
        }
    }
    
    return authheader;
}


int growl_tcp_register( const char *const server , const char *const appname , const char **const notifications , const int notifications_count ,
        const char *const password, const char* const icon  )
{
    int sock = -1;
    int i=0;
    char *authheader;
    char *iconid = NULL;
    FILE *iconfile = NULL;
    size_t iconsize;
    uint8_t buffer[1024];
    
    growl_init();
    authheader = growl_generate_authheader_alloc(password);
    sock = growl_tcp_open(server);
    if (sock == -1) goto leave;
    if (icon) {
        size_t bytes_read;
        md5_context md5ctx;
        char md5tmp[20];
        iconfile = fopen(icon, "rb");
        if (iconfile) {
            fseek(iconfile, 0, SEEK_END);
            iconsize = ftell(iconfile);
            fseek(iconfile, 0, SEEK_SET);
            memset(md5tmp, 0, sizeof(md5tmp));
            md5_starts(&md5ctx);
            while (!feof(iconfile)) {
                bytes_read = fread(buffer, 1, 1024, iconfile);
                if (bytes_read) md5_update(&md5ctx, buffer, bytes_read);
            }
            fseek(iconfile, 0, SEEK_SET);
            md5_finish(&md5ctx, md5tmp);
            iconid = string_to_hex_alloc(md5tmp, 16);
        }
    }

    growl_tcp_write(sock, "GNTP/1.0 REGISTER NONE %s", authheader ? authheader : "");
    growl_tcp_write(sock, "Application-Name: %s", appname);
    if(iconid) 
    {
        growl_tcp_write(sock, "Application-Icon: x-growl-resource://%s", iconid);	
    }
    else if(icon)
    {
        growl_tcp_write(sock, "Application-Icon: %s", icon );	
    }
    growl_tcp_write(sock, "Notifications-Count: %d", notifications_count);
    growl_tcp_write(sock, "" );

    for(i=0;i<notifications_count;i++)
    {
        growl_tcp_write(sock, "Notification-Name: %s", notifications[i]);
        growl_tcp_write(sock, "Notification-Display-Name: %s", notifications[i]);
        growl_tcp_write(sock, "Notification-Enabled: True" );
        if(iconid) 
        {
            growl_tcp_write(sock, "Notification-Icon: x-growl-resource://%s", iconid);	
        }
        else if(icon)
        {
            growl_tcp_write(sock, "Notification-Icon: %s", icon );	
        }
        growl_tcp_write(sock, "" );
    }
   
    if (iconid) 
    {
        growl_tcp_write(sock, "Identifier: %s", iconid);
        growl_tcp_write(sock, "Length: %d", iconsize);
        growl_tcp_write(sock, "" );

        while (!feof(iconfile)) 
        {
            size_t bytes_read = fread(buffer, 1, 1024, iconfile);
            if (bytes_read) growl_tcp_write_raw(sock, buffer, bytes_read);
        }
        growl_tcp_write(sock, "" );
        
    }
    growl_tcp_write(sock, "" );

    while (1)
    {
        char* line = growl_tcp_read(sock);
        if (!line) {
            growl_tcp_close(sock);
            sock = -1;
            goto leave;
        } 
        else 
        {
            int len = strlen(line);
            /* fprintf(stderr, "%s\n", line); */
            if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0) 
            {
                if (strncmp(line + 15, " NONE", 5) != 0)
                {
                    fprintf(stderr, "failed to register notification\n");
                    PhFree(line);
                    goto leave;
                }
            }
            PhFree(line);
            if (len == 0) break;
        }
    }
    growl_tcp_close(sock);
    sock = 0;

    leave:
    if (iconfile) fclose(iconfile);
    if (iconid) PhFree(iconid);
    if (authheader) PhFree(authheader);

    return (sock == 0) ? 0 : -1;
}


int growl_tcp_notify( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const password, const char* const url, const char* const icon)
{
    int sock = -1;

    char *authheader = growl_generate_authheader_alloc(password);
    char *iconid = NULL;
    FILE *iconfile = NULL;
    size_t iconsize;
    uint8_t buffer[1024];
    
    growl_init();

    sock = growl_tcp_open(server);
    if (sock == -1) goto leave;

    if (icon) 
    {
        size_t bytes_read;
        md5_context md5ctx;
        char md5tmp[20];
        iconfile = fopen(icon, "rb");
        if (iconfile)
        {
            fseek(iconfile, 0, SEEK_END);
            iconsize = ftell(iconfile);
            fseek(iconfile, 0, SEEK_SET);
            memset(md5tmp, 0, sizeof(md5tmp));
            md5_starts(&md5ctx);
            while (!feof(iconfile))
            {
                bytes_read = fread(buffer, 1, 1024, iconfile);
                if (bytes_read) md5_update(&md5ctx, buffer, bytes_read);
            }
            fseek(iconfile, 0, SEEK_SET);
            md5_finish(&md5ctx, md5tmp);
            iconid = string_to_hex_alloc(md5tmp, 16);
        }
    }

    growl_tcp_write(sock, "GNTP/1.0 NOTIFY NONE %s", authheader ? authheader : "");
    growl_tcp_write(sock, "Application-Name: %s", appname);
    growl_tcp_write(sock, "Notification-Name: %s", notify);
    growl_tcp_write(sock, "Notification-Title: %s", title);
    growl_tcp_write(sock, "Notification-Text: %s", message);
    if(iconid) 
    {
        growl_tcp_write(sock, "Notification-Icon: x-growl-resource://%s", iconid);	
    }
    else if(icon)
    {
        growl_tcp_write(sock, "Notification-Icon: %s", icon );	
    }
    if (url) growl_tcp_write(sock, "Notification-Callback-Target: %s", url  );
    
    if (iconid)
    {
        growl_tcp_write(sock, "Identifier: %s", iconid);
        growl_tcp_write(sock, "Length: %d", iconsize);
        growl_tcp_write(sock, "");
        while (!feof(iconfile)) 
        {
            size_t bytes_read = fread(buffer, 1, 1024, iconfile);
            if (bytes_read) growl_tcp_write_raw(sock, buffer, bytes_read);
        }
        growl_tcp_write(sock, "" );
    }
    growl_tcp_write(sock, "");
    
    
    while (1) 
    {
        char* line = growl_tcp_read(sock);
        if (!line) 
        {
            growl_tcp_close(sock);
            sock = -1;
            goto leave;
        } else 
        {
            int len = strlen(line);
            /* fprintf(stderr, "%s\n", line); */
            if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0) 
            {
                if (strncmp(line + 15, " NONE", 5) != 0)
                {
                    fprintf(stderr, "failed to post notification\n");
                    PhFree(line);
                    goto leave;
                }
            }
            PhFree(line);
            if (len == 0) break;
        }
    }
    growl_tcp_close(sock);
    sock = 0;

leave:
    if (iconfile) fclose(iconfile);
    if (iconid) PhFree(iconid);
    if (authheader) PhFree(authheader);

    return (sock == 0) ? 0 : -1;
}



int growl( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url )
{		
    int rc = growl_tcp_register(  server ,  appname ,  (const char **const)&notify , 1 , password, icon  );
    if( rc == 0 )
    {
        rc = growl_tcp_notify( server, appname, notify, title,  message , password, url, icon );
    }
    return rc;
}


void growl_append_md5( unsigned char *const data , const int data_length , const char *const password )
{
    md5_context md5ctx;
    char md5tmp[20];

    memset(md5tmp, 0, sizeof(md5tmp));
    md5_starts(&md5ctx);
    md5_update(&md5ctx, (uint8_t*)data, data_length );
    if(password != NULL)
    {
        md5_update(&md5ctx, (uint8_t*)password, strlen(password));
    }
    md5_finish(&md5ctx, (uint8_t*)md5tmp);

    memcpy( data + data_length , md5tmp , 16 );
}


int growl_udp_register( const char *const server , const char *const appname , const char **const notifications , const int notifications_count , const char *const password  )
{
    int register_header_length = 22+strlen(appname);
    unsigned char *data;
    int pointer = 0;
    int rc = 0;
    int i=0;

    uint8_t GROWL_PROTOCOL_VERSION  = 1;
    uint8_t GROWL_TYPE_REGISTRATION = 0;

    uint16_t appname_length = ntohs(strlen(appname));
    uint8_t _notifications_count = notifications_count;
    uint8_t default_notifications_count = notifications_count;
    uint8_t j;

    growl_init();

    for(i=0;i<notifications_count;i++)
    {
        register_header_length += 3 + strlen(notifications[i]);
    }	
    data = (unsigned char*)PhAllocateSafe(register_header_length);
    if (!data) return -1;
    memset( data , 0 ,  register_header_length );


    pointer = 0;
    memcpy( data + pointer , &GROWL_PROTOCOL_VERSION , 1 );	
    pointer++;
    memcpy( data + pointer , &GROWL_TYPE_REGISTRATION , 1 );
    pointer++;
    memcpy( data + pointer , &appname_length , 2 );	
    pointer += 2;
    memcpy( data + pointer , &_notifications_count , 1 );	
    pointer++;
    memcpy( data + pointer, &default_notifications_count , 1 );	
    pointer++;
    sprintf( (char*)data + pointer , "%s" , appname );
    pointer += strlen(appname);

    for(i=0;i<notifications_count;i++)
    {
        uint16_t notify_length = ntohs(strlen(notifications[i]));
        memcpy( data + pointer, &notify_length , 2 );		
        pointer +=2;
        sprintf( (char*)data + pointer , "%s" , notifications[i] );
        pointer += strlen(notifications[i]);
    } 

    for(j=0;j<notifications_count;j++)
    {
        memcpy( data + pointer , &j , 1 );
        pointer++;
    }

    growl_append_md5( data , pointer , password );
    pointer += 16;

    rc = growl_tcp_datagram( server , data , pointer );
    PhFree(data);
    return rc;
}


int growl_udp_notify( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const password )
{
    int notify_header_length = 28 + strlen(appname)+strlen(notify)+strlen(message)+strlen(title);
    unsigned char *data = (unsigned char*)PhAllocateSafe(notify_header_length);
    int pointer = 0;
    int rc = 0;

    uint8_t GROWL_PROTOCOL_VERSION  = 1;
    uint8_t GROWL_TYPE_NOTIFICATION = 1;

    uint16_t flags = ntohs(0);
    uint16_t appname_length = ntohs(strlen(appname));
    uint16_t notify_length = ntohs(strlen(notify));
    uint16_t title_length = ntohs(strlen(title));
    uint16_t message_length = ntohs(strlen(message));

    if (!data) return -1;

    growl_init();
    memset( data , 0 ,  notify_header_length );
    
    pointer = 0;
    memcpy( data + pointer , &GROWL_PROTOCOL_VERSION , 1 );	
    pointer++;
    memcpy( data + pointer , &GROWL_TYPE_NOTIFICATION , 1 );
    pointer++;
    memcpy( data + pointer , &flags , 2 );
    pointer += 2;
    memcpy( data + pointer , &notify_length , 2 );	
    pointer += 2;
    memcpy( data + pointer , &title_length , 2 );	
    pointer += 2;
    memcpy( data + pointer , &message_length , 2 );	
    pointer += 2;
    memcpy( data + pointer , &appname_length , 2 );	
    pointer += 2;
    strcpy( (char*)data + pointer , notify );
    pointer += strlen(notify);
    strcpy( (char*)data + pointer , title );
    pointer += strlen(title);
    strcpy( (char*)data + pointer , message );
    pointer += strlen(message);
    strcpy( (char*)data + pointer , appname );
    pointer += strlen(appname);


    growl_append_md5( data , pointer , password );
    pointer += 16;


    rc = growl_tcp_datagram( server , data , pointer );
    PhFree(data);
    return rc;
}


int growl_udp( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url )
{
    int rc = growl_udp_register(  server ,  appname ,  (const char **const)&notify , 1 , password  );
    if( rc == 0 )
    {
        rc = growl_udp_notify( server, appname, notify, title,  message , password );
    }
    return rc;
}

char *gntp_send_license_text =
"[The \"BSD licence\"]\r\n"
"Copyright (c) 2009-2010 Yasuhiro Matsumoto\r\n"
"All rights reserved.\r\n"
"\r\n"
"Redistribution and use in source and binary forms, with or without\r\n"
"modification, are permitted provided that the following conditions\r\n"
"are met:\r\n"
"\r\n"
" 1. Redistributions of source code must retain the above copyright\r\n"
"    notice, this list of conditions and the following disclaimer.\r\n"
" 2. Redistributions in binary form must reproduce the above copyright\r\n"
"    notice, this list of conditions and the following disclaimer in the\r\n"
"    documentation and/or other materials provided with the distribution.\r\n"
" 3. The name of the author may not be used to endorse or promote products\r\n"
"    derived from this software without specific prior written permission.\r\n"
"\r\n"
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR\r\n"
"IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES\r\n"
"OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.\r\n"
"IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,\r\n"
"INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT\r\n"
"NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\r\n"
"DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\r\n"
"THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\r\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF\r\n"
"THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\r\n"
;

/* 
	TODO:
	Make makes not malloc on construction
*/

#ifndef JLNET_H
#define JLNET_H
/* Deps */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
/* Libs */
#include <jlstd/jlstd.h>
/* Global */
bool JL_RUNNING    = false;
bool JL_REUSE_PORT = true;
/* Defines */
#define JL_SLEEP 33
#define JL_HEADER_BUFFER 2048
#define JL_MAX_CONNECTIONS 128
/* Status Codes */
char* JL_200 = "200 OK";
char* JL_403 = "403 Forbidden";
char* JL_404 = "404 Not Found";
/* Core */

typedef struct jlserver
{
	int fd;
	struct addrinfo hints;
	struct addrinfo* service;
	struct timespec ts;
	jlmap* routes;
} jlserver;

typedef struct jlclient
{
	int fd;
	struct sockaddr_storage addr;
	socklen_t size;
} jlclient;

typedef struct jlrequest
{
	char raw[ JL_HEADER_BUFFER ];
	char method[ 255 ];
	char url[ 255 ];
	char version[ 255 ];
	char resource[ 255 ]; // The file to be read.
	jlmap* body;
} jlrequest;

typedef struct jlresponse
{
	char raw[ JL_HEADER_BUFFER ];
	char ext[ 64 ];
	char mime[ 128 ];
	char status[ 64 ];
	jlmap* body; /* For user defined attributes / template variables */
} jlresponse;

typedef struct jlroute
{
	char method[ 255 ];
	char url[ 255 ];
	char* (*execute)( jlrequest* req, jlresponse* res );
} jlroute;

void jlsignal_listen( int sig );

jlclient jlclient_ctor( int fd, struct sockaddr_storage addr, socklen_t size );
void jlclient_dtor( jlclient* cl );

jlserver jlserver_ctor( char* port );
void jlserver_dtor( jlserver* sv );
void jlserver_listen( jlserver* sv );
jlclient jlserver_onconnect( jlserver* sv );  
void jlserver_ondisconnect( jlclient* cl );
jlrequest jlserver_onread( jlclient* cl );
jlresponse jlserver_onrespond( jlserver* sv, jlrequest* req );
void jlserver_onwrite( jlclient* cl, jlresponse* res );

jlrequest jlrequest_ctor();
void jlrequest_init( jlrequest* req );
void jlrequest_dtor( jlrequest* req );

jlresponse jlresponse_ctor();
void jlresponse_dtor( jlresponse* res );

jlroute jlroute_ctor( char* method, char* url, char* (*execute)( jlrequest* req, jlresponse* res ) );
void jlroute_dtor( jlroute* route );

/* HELPERS */
bool jl_match( char** s, char* t );
void jl_ws( char** s );
bool jl_isletter( char** s );
bool jl_isnumber( char** s );
bool jl_isjson( char** s );
bool jl_isformdata( char** s );
jlmap* jl_body( char** s );
bool jl_isfile( char** f );
void jl_readfile( char** f, char** buffer );
void jl_resheader( jlrequest* req, jlresponse* res );
void jl_buildresheader( jlrequest* req, jlresponse* res );
/* TOKENS */
bool jl_word( char** s, char* buffer );
bool jl_number( char** s, char* buffer );
void jl_stringr( char** s, char* buffer );
bool jl_string( char** s, char* buffer );
bool jl_slash( char** s, char* buffer );
bool jl_colon( char** s );
bool jl_comma( char** s );
bool jl_lbrace( char** s );
bool jl_rbrace( char** s );
bool jl_dash( char** s );
bool jl_semicolon( char** s );
bool jl_equals( char** s );
bool jl_doublequote( char** s );
/* RULES */
bool jl_url( char** s, char* buffer );
bool jl_httpversion( char **s, char* buffer );
bool jl_keyvalue( char** s, char* key, char* value );
bool jl_keyvaluer( char** s, jlmap* map );
bool jl_attr( char** s, char* key, char* value );
bool jl_attrr( char** s, jlmap* map );
bool jl_json( char** s, jlmap* map );
bool jl_formdata( char** s, jlmap* map );
/* KEYWORDS */
bool jl_httpmethod( char** s, char* buffer );
bool jl_ext( char **s, char *buffer ); // Parse file extension
bool jl_mime( char **s, char *buffer ); 
/* SKIPS */
void jl_skipheader( char** s );
/* API */
void jl_setroute( jlserver* sv, jlroute* route );
bool jl_isroute( jlserver* sv, char* url );
jlroute* jl_getroute( jlserver* sv, char* url );
#endif // JLNET_H
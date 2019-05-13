#include "../include/jlnet/jlnet.h"

void jlsignal_listen( int sig )
{
	if( sig == SIGINT )
	{
		JL_RUNNING = false;
	}
}

jlclient jlclient_ctor( int fd, struct sockaddr_storage addr, socklen_t size )
{
	jlclient cl;
	cl.fd   = fd;
	cl.addr = addr;
	cl.size = size;
	return cl;
}

void jlclient_dtor( jlclient* cl )
{

}

jlserver jlserver_ctor( char* port )
{
	jlserver sv;
	memset( &sv.hints, 0, sizeof( sv.hints ) );
	sv.hints.ai_family   = AF_INET;
	sv.hints.ai_socktype = SOCK_STREAM;
	getaddrinfo( NULL, port, &sv.hints, &sv.service );
	sv.fd = socket( sv.service->ai_family, sv.service->ai_socktype, sv.service->ai_protocol );
	fcntl( sv.fd, F_SETFL, O_NONBLOCK );
	setsockopt( sv.fd, SOL_SOCKET, SO_REUSEADDR, &JL_REUSE_PORT, sizeof( JL_REUSE_PORT ) );
	setsockopt( sv.fd, SOL_SOCKET, SO_REUSEPORT, &JL_REUSE_PORT, sizeof( JL_REUSE_PORT ) );
	bind( sv.fd, sv.service->ai_addr, sv.service->ai_addrlen );
	listen( sv.fd, JL_MAX_CONNECTIONS );
	sv.ts.tv_sec  = JL_SLEEP / 1000;
	sv.ts.tv_nsec = ( JL_SLEEP % 1000 ) * 1000000;
	sv.routes = jlmap_ctor( 128 );
	return sv;
}

void jlserver_dtor( jlserver* sv )
{
	printf("\nServer Stopped\n");
	jlmap_dtor( &sv->routes );
}

void jlserver_listen( jlserver* sv )
{
	printf("Server Started\n");
	signal( SIGINT, jlsignal_listen );
	JL_RUNNING = true;
	while( JL_RUNNING )
	{
		jlclient cl        = jlserver_onconnect( sv );
		if( cl.fd > 0 )
		{
			jlrequest req  = jlserver_onread( &cl );
			jlresponse res = jlserver_onrespond( sv, &req );
			jlserver_onwrite( &cl, &res );
			jlserver_ondisconnect( &cl );
		}
		nanosleep( &sv->ts, NULL );
	}
	jlserver_dtor( sv );
}

jlclient jlserver_onconnect( jlserver* sv )
{
	int clFd = 0;
	struct sockaddr_storage clAddr;
	socklen_t clSize = sizeof( clFd );
	jlclient cl;
	clFd = accept( sv->fd, ( struct sockaddr* ) &clAddr, &clSize );
	cl = jlclient_ctor( clFd, clAddr, clSize );
	return cl;
}

jlrequest jlserver_onread( jlclient* cl )
{
	jlrequest req;
	read( cl->fd, req.raw, JL_HEADER_BUFFER );
	jlrequest_init( &req );
	char* reqP   = req.raw;
	char** reqDp = &reqP;
	jl_httpmethod( reqDp, req.method );
	jl_url( reqDp, req.url );
	jl_httpversion( reqDp, req.version );
	jl_skipheader( reqDp );
	req.body = jl_body( reqDp );
	return req;
}

jlresponse jlserver_onrespond( jlserver* sv, jlrequest* req )
{
	jlresponse res;
	jlroute* route  = jl_getroute( sv, req->url );
	res.body        = jlmap_ctor( 64 );
	char *resRawP   = res.raw;
	char **resRawDp = &resRawP;

	if( route != NULL && route->execute != NULL )
	{ 
		char* resource = route->execute( req, &res );
		strcpy( req->resource, resource );
	}
	else 
	{
		strcpy( req->resource, req->url );
	}
	char *reqResourceP   = req->resource;
	char **reqResourceDp = &reqResourceP;
	bool exists = jl_isfile( reqResourceDp );

	if( !exists )
		strcpy( res.status, JL_404 );
	else
		strcpy( res.status, JL_200 );

	jl_resheader( req, &res );
	jl_readfile( reqResourceDp, resRawDp );
	printf("RESPONSE AFTER PARSING:\n%s\n", res.raw );
	jlmap_dtor( &req->body );
	return res;
}

void jlserver_onwrite( jlclient* cl, jlresponse* res )
{
	write( cl->fd, res->raw, strlen( res->raw ) );
	jlmap_dtor( &res->body );
}

void jlserver_ondisconnect( jlclient* cl )
{
	close( cl->fd );
}

jlrequest jlrequest_ctor()
{
	jlrequest req;
	memset( req.method, 0, sizeof( req.method ) );
	memset( req.url, 0, sizeof( req.url ) );
	return req;
}

void jlrequest_init( jlrequest* req )
{
	memset( req->method, 0, sizeof( req->method ) );
	memset( req->url, 0, sizeof( req->url ) );
	memset( req->version, 0, sizeof( req->version ) );
}

void jlrequest_dtor( jlrequest* req )
{

}

jlroute jlroute_ctor( char* method, char* url, char* (*execute)( jlrequest* req, jlresponse* res ) )
{
	jlroute route;
	strcpy( route.method, method );
	strcpy( route.url, url );
	route.execute = execute;
	return route;
}

void jlroute_dtor( jlroute* route )
{

}

bool jl_match( char** s, char* t )
{
	if( strncmp( *s, t, strlen( t ) ) == 0 )
	{
		*s = *s + strlen( t );
		return true;
	}
	return false;
}

void jl_ws( char** s )
{
	if( *s[0] == ' ' || *s[0] == '\t' || *s[0] == '\n' || *s[0] == '\r' )
	{
		*s = *s+1;
		jl_ws( s );
	}
}

bool jl_isletter( char** s )
{
	char c = *s[0];
	return c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F' || c == 'G' || c == 'H' ||
	c == 'I' || c == 'J' || c == 'K' || c == 'L' || c == 'M' || c == 'N' || c == 'O' || c == 'P' ||
	c == 'Q' || c == 'R' || c == 'S' || c == 'T' || c == 'U' || c == 'V' || c == 'W' || c == 'X' ||
	c == 'Y' || c == 'Z' || c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f' ||
	c == 'g' || c == 'h' || c == 'i' || c == 'j' || c == 'k' || c == 'l' || c == 'm' || c == 'n' ||
	c == 'o' || c == 'p' || c == 'q' || c == 'r' || c == 's' || c == 't' || c == 'u' || c == 'v' ||
	c == 'w' || c == 'x' || c == 'y' || c == 'z' || c == '.' || c == '@'; 
}

bool jl_isnumber( char** s )
{
	char c = *s[0];
	return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9' || c == '.';
}

bool jl_isjson( char** s )
{
	char* str   = *s;
	char first = str[0];
	char last  = str[ strlen( str )-1 ];
	if( last != '}' ) last = str[ strlen( str )-2 ];
	return first == '{' && last == '}';
}

bool jl_isformdata( char** s )
{
	return false;
}

jlmap* jl_body( char** s )
{
	jlmap* body = jlmap_ctor( 32 );
	char* old = *s;
	bool success = jl_json( s, body );
	if( !success ){ *s = old; success = jl_formdata( s, body ); }
	if( !success ){ *s = old; }
	return body;
}

void jl_resheader( jlrequest* req, jlresponse* res )
{
	memset( res->raw, 0, sizeof( res->raw ) );
	memset( res->ext, 0, sizeof( res->ext ) );
	memset( res->mime, 0, sizeof( res->mime ) );
	char *resourceP   = req->resource;
	char **resourceDp = &resourceP;
	jl_ext( resourceDp, res->ext );
	char *extP   = res->ext;
	char **extDp = &extP;
	jl_mime( extDp, res->mime );
	jl_buildresheader( req, res );
}

void jl_buildresheader( jlrequest* req, jlresponse* res )
{
	strcat( res->raw, req->version );
	strcat( res->raw, " " );
	strcat( res->raw, res->status );
	strcat( res->raw, "\r\n" );
	strcat( res->raw, "Server: Jolt\r\n" );
	strcat( res->raw, "Content-Type: " );
	strcat( res->raw, res->mime );
	strcat( res->raw, "\r\n\r\n" );
}

bool jl_isfile( char** f )
{
	if( *f[0] == '/' ) *f = *f + 1;
	FILE *fp = fopen( *f, "r" );
	if( fp == NULL ) return false;
	fclose( fp );
	fp = NULL;
	return true;
}

void jl_readfile( char** f, char** buffer )
{
	if( *f[0] == '/' ) *f = *f+1;
	FILE* fp = fopen( *f, "r" );
	if( fp == NULL ) return;

	for( int i = 0 ; i != EOF; i = fgetc( fp ) )
	{
		char s[2] = {i,'\0'};
		strcat( *buffer, s );
	}
	fclose( fp );
	fp = NULL;
}

bool jl_word( char** s, char* buffer )
{
	if( jl_isletter( s ) )
	{
		buffer[ strlen( buffer ) ] = *s[0];
		buffer[ strlen( buffer ) ] = '\0';
		*s = *s+1;
		jl_word( s, buffer );
		return true;
	}
	return false;
}

bool jl_number( char** s, char* buffer )
{
	if( jl_isnumber( s ) )
	{
		if( buffer != NULL )
		{
			buffer[ strlen( buffer ) ] = *s[0];
			buffer[ strlen( buffer ) ] = '\0';
		}
		*s = *s+1;
		jl_number( s, buffer );
		return true;
	}
	return false;
}

void jl_stringr( char** s, char* buffer )
{
	if( jl_word( s, buffer ) || jl_number( s, buffer ) ) jl_stringr( s, buffer );
}

bool jl_string( char** s, char* buffer )
{
	if( jl_doublequote( s ) )
	{
		jl_stringr( s, buffer );
		if( jl_doublequote( s ) ) return true;
		return false;
	}
	return false;
}

bool jl_slash( char** s, char* buffer )
{
	char* old = *s;
	if( jl_match( s, "/" ) )
	{
		buffer[ strlen( buffer ) ] = old[0];
		buffer[ strlen( buffer ) ] = '\0';
		return true;
	}
	return false;
}

bool jl_colon( char** s )
{
	if( jl_match( s, ":" ) ) return true;
	return false;
}

bool jl_comma( char** s )
{
	if( jl_match( s, "," ) ) return true;
	return false;
}

bool jl_lbrace( char** s )
{
	if( jl_match( s, "{" ) ) return true;
	return false;
}

bool jl_rbrace( char** s )
{
	if( jl_match( s, "}" ) ) return true;
	return false;
}

bool jl_dash( char** s )
{
	if( jl_match( s, "-" ) ) return true;
	return false;
}

bool jl_semicolon( char** s )
{
	if( jl_match( s, ";" ) ) return true;
	return false;
}

bool jl_equals( char** s )
{
	if( jl_match( s, "=" ) ) return true;
	return false;
}

bool jl_doublequote( char** s )
{
	if( jl_match( s, "\"" ) ) return true;
	return false;
}

bool jl_url( char** s, char* buffer )
{
	jl_ws( s );
	if( jl_slash( s, buffer ) )
	{
		if( jl_isletter( s ) )
		{
			jl_word( s, buffer );
			return jl_url( s, buffer );
		}
		else if( jl_isnumber( s ) )
		{
			jl_number( s, buffer );
			return jl_url( s, buffer );
		}
		return true;
	}
	return false;
}

bool jl_httpversion( char **s, char* buffer )
{
	jl_ws( s );
	if( jl_word( s, buffer ) && jl_slash( s, buffer ) && jl_number( s, buffer ) )
	{
		return true;
	}
	return false;
}

bool jl_keyvalue( char** s, char* key, char* value )
{
	char* old = *s;
	jl_ws( s );
	if( jl_string( s, key ) )
	{
		if( jl_colon( s ) )
		{
			old = *s;
			bool success = jl_string( s, value );
			if( !success ){ *s = old; success = jl_number( s, value ); } // Rewind + check if number
			jl_comma( s );
			return true;
		}
		*s = old;
		return false;
	}
	return false;
}

bool jl_keyvaluer( char** s, jlmap* map )
{
	jl_ws( s );
	char key[256] = { 0 }, value[256] = { 0 };
	char* keyP   = key;
	char* valueP = value;
	
	if( jl_keyvalue( s, keyP, valueP ) )
	{
		jlmap_set( map, key, value );
		jl_keyvaluer( s, map );
		return true;
	}
	return false;
}

bool jl_attr( char** s, char* key, char* value )
{
	char* old = *s;
	jl_ws( s );
	if( jl_match( s, "----------------------------" ) && jl_isnumber( s ) )
	{
		jl_number( s, NULL );
		jl_ws(s);
		if( jl_match( s, "Content-Disposition: form-data;" ) )
		{
			jl_ws(s);
			if( jl_match( s, "name=" ) )
			{
				jl_string( s, key );
				jl_ws(s);
				old = *s;
				bool success = jl_word( s, value );
				if( !success ){ *s = old; success = jl_number( s, value ); }
				return true;
			}
			return false;
		}
		return false;
	}
	*s = old;
	return false;
}

bool jl_attrr( char** s, jlmap* map )
{
	char key[256] = { 0 }, value[256] = { 0 };
	char* keyP   = key;
	char* valueP = value;	

	if( jl_attr( s, keyP, valueP ) )
	{
		jlmap_set( map, key, value );
		jl_attrr( s, map );
		return true;
	}
	return false;
}

bool jl_json( char** s, jlmap* map )
{
	jl_ws( s );
	if( jl_lbrace( s ) )
	{
		if( jl_keyvaluer( s, map ) )
		{
			jl_rbrace( s );
			return true;
		}
		return false;
	}
	return false;
}

bool jl_formdata( char** s, jlmap* map )
{
	jl_ws( s );

	if( *s[0] == '-' )
	{
		if( jl_attrr( s, map ) )
		{
			if( jl_dash( s ) && jl_dash( s ) ) return true;
			return false;
		}
		return false;
	}
	return false;
}

bool jl_httpmethod( char** s, char* buffer )
{
	char* old = *s;
	jl_ws( s );
	if( jl_match( s, "GET" ) || jl_match( s, "POST" ) )
	{
		strncpy( buffer, old, *s-old );
		return true;
	}
	return false;
}

bool jl_ext( char **s, char *buffer )
{
	char* old = *s;
	*s = *s + strlen( *s );
	char* end = *s;

	for( ; *s[0] != '.'; *s = *s - 1 )
	{
		if( old == *s )
		{
			*s = old;
			return false;
		}
	}
	strncpy( buffer, *s, end-*s );
	*s = old;
	return true;
}

bool jl_mime( char **s, char *buffer )
{
	char* old = *s;
	if( jl_match( s, ".html" ) ){ strcpy( buffer, "text/html" ); return true; }
	else if( jl_match( s, ".css" ) ){ strcpy( buffer, "text/css" ); return true; }		
	else if( jl_match( s, ".js" ) ){ strcpy( buffer, "application/javascript" ); return true; }
	else if( jl_match( s, ".json" ) ){ strcpy( buffer, "application/json" ); return true; }
	*s = old;
	return false;
}

void jl_skipheader( char** s )
{
	while( !jl_match( s, "\r\n\r\n" ) ) *s = *s + 1;
}
/* API */
void jl_setroute( jlserver* sv, jlroute* route )
{
	jlmap_set( sv->routes, route->url, route );
}

bool jl_isroute( jlserver* sv, char* url )
{
	jlroute* route = (jlroute*) jlmap_get( sv->routes, url );
	return route != NULL;
}

jlroute* jl_getroute( jlserver* sv, char* url )
{
	return (jlroute*) jlmap_get( sv->routes, url );
}
#include <jlnet/jlnet.h>

char *Home( jlrequest* req, jlresponse* res )
{
	printf("HIT: /home\n");
	return "html/index.html";
}

int main( int argc, char** argv )
{
	jlserver sv = jlserver_ctor( "8080" );
	jlroute route = jlroute_ctor( "GET", "/home", Home );
	jl_setroute( &sv, &route );
	jlserver_listen( &sv );
	return 0;
}
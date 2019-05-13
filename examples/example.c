#include <jlnet/jlnet.h>

int main( int argc, char** argv )
{
	jlserver sv = jlserver_ctor( "8080" );
	jlserver_listen( &sv );
	return 0;
}
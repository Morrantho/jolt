#Jolt

## Jolt is a C Web Server that aims to make writing a webapp in C as simple as possible. It currently supports GET, POST, PUT, PATCH and DELETE Http methods. Currently there is no template engine, so you'll have to write static HTML files.

## Installation / Dependencies:

#### 1. Jolt depends on the jlstd library, which merely consists of a few common data structures. You can find an build it for your system here: https://github.com/Morrantho/jlstd

#### 2. By running the build.sh at the root of the directory, Jolt is compiled into a shared library and installed in: /usr/local/include and /usr/local/bin directories.

#### 3. grep for libjlnet.so in /usr/local/bin. If you see output, it compiled successfully.

#### 4. Try to build the example in examples directory by running its example.sh. This will link both jlstd and jlnet, compiling the example.c file

## Example:

```C
#include <jlnet/jlnet.h> // Include jolt header

char *Home( jlrequest* req, jlresponse* res )
{
	return "html/index.html"; // Render index.html
}

int main( int argc, char** argv )
{
	jlserver sv = jlserver_ctor( "8080" ); // Listen on 8080.
	jlroute route = jlroute_ctor( "GET", "/home", Home ); // Create a GET route for "/home" that colls Home(). 
	jl_setroute( &sv, &route ); // Registers the above route into the server routes for lookup.
	jlserver_listen( &sv ); // Runs the server.
	return 0;
}
```

#### Visit localhost:8080/home in your favorite browser. You should see a Red "Hello World".

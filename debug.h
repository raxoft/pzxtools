// $Id$

// Debug support.

#ifndef DEBUG_H
#define DEBUG_H 1

// Just a lightweight version of some of the stuff I am used to.

#include <cstdlib>
#include <cstdio>

#define hope(c)         while(!(c)){std::fprintf(stderr,"hope: %s\n",#c);std::abort();}
#define fail(f,...)     (std::fprintf(stderr,"error: " f "\n",##__VA_ARGS__),std::exit(EXIT_FAILURE))
#define warn(f,...)     (std::fprintf(stderr,"warning: " f "\n",##__VA_ARGS__))
#define inform(f,...)   (std::fprintf(stderr,"info: " f "\n",##__VA_ARGS__))

#endif // DEBUG_H

// $Id: debug.h 315 2007-06-25 20:49:34Z patrik $

/**
 * @file Debug support.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#ifndef DEBUG_H
#define DEBUG_H 1

#ifndef SYSDEFS_H
#include "sysdefs.h"
#endif

#include <cstdlib>
#include <cstdio>

// Just a lightweight version of some of the stuff I am used to.

#ifdef DEBUG
#define hope(c)         while(!(c)){std::fprintf(stderr,"hope: %s (%s:%u)\n",#c,__FILE__,__LINE__);std::abort();}
#else
#define hope(c)         void(0)
#endif

#define fail(f,...)     (std::fprintf(stderr,"error: " f "\n",##__VA_ARGS__),std::exit(EXIT_FAILURE))
#define warn(f,...)     (std::fprintf(stderr,"warning: " f "\n",##__VA_ARGS__))
#define inform(f,...)   (std::fprintf(stderr,"info: " f "\n",##__VA_ARGS__))

#endif // DEBUG_H

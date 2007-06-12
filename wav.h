// $Id$

// WAV stuff.

#ifndef WAV_H
#define WAV_H 1

#include <cstdio>

#ifndef TYPES_H
#include "types.h"
#endif

// Interface.

void wav_open( FILE * file, const uint numerator, const uint denominator ) ;
void wav_close( void ) ;

void wav_out( const uint duration, const bool level ) ;

#endif // WAV_H

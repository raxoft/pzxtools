// $Id: wav.h 302 2007-06-15 07:37:58Z patrik $

/**
 * @file WAV stuff.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#ifndef WAV_H
#define WAV_H 1

#include <cstdio>

#ifndef ENDIAN_H
#include "endian.h"
#endif

// WAV chunk tags.

const uint WAV_HEADER   = TAG_NAME('R','I','F','F') ;
const uint WAV_WAVE     = TAG_NAME('W','A','V','E') ;
const uint WAV_FORMAT   = TAG_NAME('f','m','t',' ') ;
const uint WAV_DATA     = TAG_NAME('d','a','t','a') ;

// Interface.

void wav_open( FILE * file, const uint numerator, const uint denominator ) ;
void wav_close( void ) ;

void wav_out( const uint duration, const bool level ) ;

#endif // WAV_H

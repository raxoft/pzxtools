// $Id: pzx.h 317 2007-06-25 20:55:26Z patrik $

/**
 * @file PZX stuff.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#ifndef PZX_H
#define PZX_H 1

#include <cstdio>

#ifndef BUFFER_H
#include "buffer.h"
#endif

// PZX version we support.

const byte PZX_MAJOR = 1 ;
const byte PZX_MINOR = 0 ;

// PZX block tags.

const uint PZX_HEADER   = TAG_NAME('P','Z','X','T') ;
const uint PZX_PULSES   = TAG_NAME('P','U','L','S') ;
const uint PZX_DATA     = TAG_NAME('D','A','T','A') ;
const uint PZX_PAUSE    = TAG_NAME('P','A','U','S') ;
const uint PZX_STOP     = TAG_NAME('S','T','O','P') ;
const uint PZX_BROWSE   = TAG_NAME('B','R','W','S') ;

// Interface.

void pzx_open( FILE * file ) ;
void pzx_close( void ) ;

void pzx_write( const void * const data, const uint size ) ;
void pzx_write_block( const uint tag, const void * const data, const uint size ) ;
void pzx_write_buffer( const uint tag, Buffer & buffer ) ;

void pzx_header( const void * const data, const uint size ) ;
void pzx_info( const void * const string, const uint size ) ;
void pzx_info( const char * const string ) ;

void pzx_store( const uint count, const uint duration ) ;
void pzx_pulse( const uint duration ) ;
void pzx_out( const uint duration, const bool level ) ;

void pzx_flush( void ) ;

void pzx_data(
    const byte * const data,
    const uint bit_count,
    const bool initial_level,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const word * const pulse_sequence_0,
    const word * const pulse_sequence_1,
    const uint tail_cycles
) ;

bool pzx_pack(
    const word * const pulses,
    const uint pulse_count,
    const bool initial_level,
    const word * const sequence_0,
    const word * const sequence_1,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const uint tail_cycles
) ;

bool pzx_pack(
    const word * const pulses,
    const uint pulse_count,
    const bool initial_level,
    const uint sequence_limit,
    const uint sequence_order,
    const uint tail_cycles
) ;

void pzx_pulses(
    const word * const pulses,
    const uint pulse_count,
    const bool initial_level,
    const uint tail_cycles
) ;

void pzx_pause( const uint duration, const bool level ) ;

void pzx_stop( const uint flags ) ;

void pzx_browse( const void * const data, const uint size ) ;
void pzx_browse( const char * const string ) ;

#endif // PZX_H

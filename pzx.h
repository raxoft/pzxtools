// $Id$

// PZX stuff.

#ifndef PZX_H
#define PZX_H 1

#include <cstdio>

#ifndef BUFFER_H
#include "buffer.h"
#endif

// PZX version we support.

const byte PZX_MAJOR = 0 ;
const byte PZX_MINOR = 3 ;

// PZX block tags.

const uint PZX_HEADER   = 'ZXTP' ;
const uint PZX_PULSES   = 'PULS' ;
const uint PZX_DATA     = 'DATA' ;
const uint PZX_PAUSE    = 'PAUS' ;
const uint PZX_STOP     = 'STOP' ;
const uint PZX_BROWSE   = 'BRWS' ;

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
    const u16 * const pulse_sequence_0,
    const u16 * const pulse_sequence_1,
    const uint tail_cycles
) ;

void pzx_pause( const uint duration, const bool level ) ;

void pzx_stop( const uint flags ) ;

void pzx_browse( const void * const data, const uint size ) ;
void pzx_browse( const char * const string ) ;

#endif // PZX_H

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

#if ( __BYTE_ORDER == __LITTLE_ENDIAN )
# define PZX_TAG(a,b,c,d)   ((d)<<24|(c)<<16|(b)<<8|(a))
#elif ( __BYTE_ORDER == __BIG_ENDIAN )
# define PZX_TAG(a,b,c,d)   ((a)<<24|(b)<<16|(c)<<8|(d))
#else
# error "Unknown __BYTE_ORDER."
#endif

const uint PZX_HEADER   = PZX_TAG('Z','X','T','P') ;
const uint PZX_PULSES   = PZX_TAG('P','U','L','S') ;
const uint PZX_DATA     = PZX_TAG('D','A','T','A') ;
const uint PZX_PAUSE    = PZX_TAG('P','A','U','S') ;
const uint PZX_STOP     = PZX_TAG('S','T','O','P') ;
const uint PZX_BROWSE   = PZX_TAG('B','R','W','S') ;

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

void pzx_pause( const uint duration, const bool level ) ;

void pzx_stop( const uint flags ) ;

void pzx_browse( const void * const data, const uint size ) ;
void pzx_browse( const char * const string ) ;

bool pzx_pack(
    const word * const data,
    const uint length,
    const bool initial_level,
    const uint sequence_limit,
    const uint tail_cycles
) ;

#endif // PZX_H

// $Id$

// CSW support.

#ifndef CSW_H
#define CSW_H 1

#ifndef BUFFER_H
#include "buffer.h"
#endif

// Interface.

uint csw_render_block( bool & level, const uint sample_rate, const byte * const data, const uint size ) ;
void csw_unpack_block( Buffer & buffer, const byte * const data, const uint size ) ;

uint csw_render_block( bool & level, const uint compression, const uint sample_rate, const byte * const data, const uint size ) ;

void csw_render( const byte * const data, const uint size ) ;

#endif // CSW_H

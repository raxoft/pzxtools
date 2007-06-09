// $Id$

// CSW support.

#ifndef CSW_H
#define CSW_H 1

#ifndef BUFFER_H
#include "buffer.h"
#endif

// Interface.

void csw_render_block(
    bool & level,
    const uint sample_rate,
    const uint pulse_count,
    const byte * const data,
    const uint size
) ;

void csw_unpack_block( Buffer & buffer, const byte * const data, const uint size ) ;

void csw_render( const byte * const data, const uint size ) ;

#endif // CSW_H

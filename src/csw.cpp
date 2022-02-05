// $Id: csw.cpp 302 2007-06-15 07:37:58Z patrik $

/**
 * @file Rendering of CSW files.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#ifndef NO_ZLIB
#include <zlib.h>
#endif

#include "csw.h"
#include "pzx.h"

/**
 * Macros for fetching little endian data from data block.
 */
//@{
#define GET1(o)     (data[o])
#define GET2(o)     (data[o]+(data[(o)+1]<<8))
#define GET3(o)     (data[o]+(data[(o)+1]<<8)+(data[(o)+2]<<16))
#define GET4(o)     (data[o]+(data[(o)+1]<<8)+(data[(o)+2]<<16)+(data[(o)+3]<<24))
//@}

/**
 * Render CSW encoded pulses to the output stream.
 */
uint csw_render_block( bool & level, const uint sample_rate, const byte * const data, const uint size )
{
    hope( sample_rate > 0 ) ;
    hope( data || size == 0 ) ;

    const byte * p = data ;
    const byte * const end = data + size ;

    // Iterate over all pulses.

    uint pulse_count = 0 ;

    while ( p < end ) {

        // Fetch the pulse duration encoded as number of samples.

        uint sample_count = *p++ ;

        // In case it is 0, fetch the 32 bit sample count.

        if ( sample_count == 0 ) {

            if ( end - p < 4 ) {
                warn( "premature end of CSW data detected" ) ;
                break ;
            }

            sample_count = *p++ ;
            sample_count += *p++ << 8 ;
            sample_count += *p++ << 16 ;
            sample_count += *p++ << 24 ;
        }

        // Now output the pulse, converting the sample count to duration in 3.5MHz T cycles first.
        //
        // Note that by rounding down we lose up to almost 1 T for every
        // pulse, but it's precise enough for our purposes.

        uquad duration = ( ( 3500000ull * sample_count ) / sample_rate ) ;
        const uint limit = 0xFFFFFFFF ;

        while ( duration > limit ) {
            pzx_out( limit, level ) ;
            duration -= limit ;
        }

        pzx_out( uint( duration ), level ) ;

        level = ! level ;

        pulse_count++ ;
    }

    return pulse_count ;
}

/**
 * Unpack given CSW block to given buffer.
 */
void csw_unpack_block( Buffer & buffer, const byte * const data, const uint size )
{
    hope( data || size == 0 ) ;

    if ( size == 0 ) {
        return ;
    }

#ifdef NO_ZLIB

    warn( "zlib support si not compiled in, so CSW Z-RLE compression is not supported" ) ;
    return ;

#else // NO_ZLIB

    // Initialize the zlib stream stucture.

    z_stream stream ;
    stream.zalloc = Z_NULL ;
    stream.zfree = Z_NULL ;
    stream.opaque = NULL ;
    stream.next_in = const_cast< byte * >( data ) ;
    stream.avail_in = size ;

    if ( inflateInit( &stream ) != Z_OK ) {
        warn( "error initializing zlib decompressor for CSW block: %s", stream.msg ? stream.msg : "unknown error" ) ;
        return ;
    }

    // Keep decompressing chunk by chunk, collecting the output in the
    // output buffer.

    int result ;

    do {

        const uint chunk_size = 16384 ;
        byte chunk[ chunk_size ] ;

        stream.next_out = chunk ;
        stream.avail_out = chunk_size ;

        result = inflate( &stream, Z_NO_FLUSH ) ;

        if ( result != Z_OK && result != Z_STREAM_END ) {
            warn( "error while decompressing CSW block: %s", stream.msg ? stream.msg : "unknown error" ) ;
            break ;
        }

        buffer.write( chunk, chunk_size - stream.avail_out ) ;

    } while ( result != Z_STREAM_END ) ;

    // Cleanup.

    inflateEnd( &stream ) ;

#endif // NO_ZLIB

}

/**
 * Render CSW encoded pulses to the output stream.
 */
uint csw_render_block( bool & level, const uint compression, const uint sample_rate, const byte * const data, const uint size )
{
    // Process the data depending on the compression.

    switch ( compression ) {
        case 1: {
            return csw_render_block( level, sample_rate, data, size ) ;
        }
        case 2: {
            Buffer buffer ;
            csw_unpack_block( buffer, data, size ) ;
            return csw_render_block( level, sample_rate, buffer.get_data(), buffer.get_data_size() ) ;
        }
        default: {
            warn( "unsupported CSW compression 0x%02x scheme", compression ) ;
            return 0 ;
        }
    }
}

/**
 * Render given CSW file to the PZX output stream.
 */
void csw_render( const byte * const data, const uint size )
{
    hope( data ) ;
    hope( size >= 0x20 ) ;

    // Check the version.

    const uint major = GET1(0x17) ;
    const uint minor = GET1(0x18) ;

    uint supported_minor = 0 ;
    uint header_size = 0x20 ;

    switch ( major ) {
        case 1: {
            supported_minor = 1 ;
            break ;
        }
        case 2: {
            header_size = 0x34 ;
            break ;
        }
        default: {
            fail( "unsupported CSW major version %u.%02u - stopping", major, minor ) ;
        }
    }

    if ( header_size > size ) {
        fail( "CSW header is incomplete" ) ;
    }

    if ( minor > supported_minor ) {
        warn( "unsupported CSW minor version %u.%02u - proceeding", major, minor ) ;
    }

    // Extract the necessary info.

    uint sample_rate = 0 ;
    uint compression = 0 ;
    uint flags = 0 ;
    uint data_offset = header_size ;

    switch ( major ) {
        case 1: {
            sample_rate = GET2(0x19) ;
            compression = GET1(0x1B) ;
            flags = GET1(0x1C) ;
            break ;
        }
        case 2: {
            sample_rate = GET4(0x19) ;
            compression = GET1(0x21) ;
            flags = GET1(0x22) ;
            data_offset += GET1(0x23) ;
            break ;
        }
    }

    // Verify sample rate.

    if ( sample_rate == 0 ) {
        fail( "invalid CSW sample rate %u", sample_rate ) ;
    }

    // Prepare data block.

    if ( data_offset > size ) {
        fail( "CSW file is incomplete" ) ;
    }

    const byte * const block = data + data_offset ;
    const uint block_size = size - data_offset ;

    // Prepare initial level.

    bool level = ( ( flags & 1 ) != 0 ) ;

    // Process the data depending on the compression.

    const uint pulse_count = csw_render_block( level, compression, sample_rate, block, block_size ) ;

    // Verify the pulse count matched.

    if ( major == 2 ) {
        const uint expected_pulse_count = GET4(0x1D) ;
        if ( pulse_count != expected_pulse_count ) {
            warn( "real CSW pulse count %u doesn't match the advertised pulse count %u", pulse_count, expected_pulse_count ) ;
        }
    }
}

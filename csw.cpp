// $Id$

// CSW support.

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

        long long unsigned duration = ( ( 3500000ull * sample_count ) / sample_rate ) ;
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
    fail( "not yet" ) ;
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

    uint pulse_count ;

    switch ( compression ) {
        case 1: {
            pulse_count = csw_render_block( level, sample_rate, block, block_size ) ;
            break ;
        }
        case 2: {
            Buffer buffer ;
            csw_unpack_block( buffer, block, block_size ) ;
            pulse_count = csw_render_block( level, sample_rate, buffer.get_data(), buffer.get_data_size() ) ;
            break ;
        }
        default: {
            fail( "invalid CSW compression 0x%02x", compression ) ;
        }
    }

    // Verify the pulse count matched.

    if ( major == 2 ) {
        const uint expected_pulse_count = GET4(0x1D) ;
        if ( pulse_count != expected_pulse_count ) {
            warn( "real CSW pulse count %u doesn't match the advertised pulse count %u", pulse_count, expected_pulse_count ) ;
        }
    }
}

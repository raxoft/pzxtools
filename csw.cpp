// $Id$

// CSW support.

#include "csw.h"
#include "pzx.h"

/**
 * Render CSW encoded pulses to the output stream.
 */
void csw_render_block( bool & level, const uint sample_rate, const uint pulse_count, const byte * const data, const uint size )
{
    hope( sample_rate > 0 ) ;
    hope( data || pulse_count == 0 ) ;

    const byte * p = data ;
    const byte * const end = data + size ;

    // Iterate over all pulses.

    for ( uint i = 0 ; i < pulse_count ; i++ ) {

        // Make sure there are still some data.

        if ( p >= end ) {
            warn( "premature end of CSW data" ) ;
            return ;
        }

        // Fetch the pulse duration encoded as number of samples.

        uint sample_count = *p++ ;

        // In case it is 0, fetch the 32 bit sample count.

        if ( sample_count == 0 ) {

            if ( end - p < 4 ) {
                warn( "premature end of CSW data" ) ;
                return ;
            }

            sample_count = *p++ ;
            sample_count += *p++ << 8 ;
            sample_count += *p++ << 16 ;
            sample_count += *p++ << 24 ;
        }

        // Now output the pulse, converting the sample count to duration in 3.5MHz T cycles first.

        long long unsigned duration = ( ( 3500000ull * duration ) / sample_rate ) ;
        const uint limit = 0xFFFFFFFF ;

        while ( duration > limit ) {
            pzx_out( limit, level ) ;
            duration -= limit ;
        }

        pzx_out( uint( duration ), level ) ;

        level = ! level ;
    }
}

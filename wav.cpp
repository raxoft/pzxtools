// $Id$

// Rendering to WAV files.

#include "wav.h"
#include "buffer.h"

namespace {

/**
 * File currently used for output, if any.
 */
FILE * output_file ;

/**
 * Buffer used for holding the complete samples.
 */
Buffer sample_buffer ;

/**
 * Numerator and denominator for converting specified durations to number of samples.
 */
//@{
uint sample_numerator ;
uint sample_denominator ;
//@}

/**
 * Duration and value of the last sample accumulated so far, both scaled by sample_numerator.
 */
//@{
uquad sample_duration ;
uquad sample_value ;
//@}

}

/**
 * Use given file for subsequent WAV output.
 */
void wav_open( FILE * file, const uint numerator, const uint denominator )
{
    hope( file ) ;
    hope( numerator > 0 ) ;
    hope( denominator > 0 ) ;

    // Remember the file.

    hope( output_file == NULL ) ;
    output_file = file ;

    // Remember the timing factors.

    sample_numerator = numerator ;
    sample_denominator = denominator ;
}

/**
 * Write given memory block of given size to output file.
 */
void wav_write( const void * const data, const uint size )
{
    hope( output_file ) ;

    // Just write everything, freaking out in case of problems.

    if ( std::fwrite( data, 1, size, output_file ) != size ) {
        fail( "error writing to file" ) ;
    }
}

/**
 * Write everything to WAV output file and stop using that file.
 */
void wav_close( void )
{
    hope( output_file ) ;

    // FIXME: write the output here.


    // Forget about the file.

    output_file = NULL ;
}

/**
 * Append pulse of given duration and given pulse level to WAV output.
 */
void wav_out( const uint duration, const bool level )
{
    // Compute how much time has passed and how much is there left
    // until the next sample starts.

    uquad time_passed = ( duration * sample_numerator ) ;
    const uquad time_left = ( sample_denominator - sample_duration ) ;

    // If we have finished current sample, output it now.

    if ( time_passed >= time_left ) {

        // Adjust the values.

        time_passed -= time_left ;
        if ( level ) {
            sample_value += time_left ;
        }

        // Output the sample.

        sample_buffer.write( u8( sample_value / sample_denominator ) ) ;

        // Prepare for next sample.

        sample_duration = 0 ;
        sample_value = 0 ;
    }

    // In case the time passed covered several more samples as well,
    // generate them now.

    for ( ; time_passed >= sample_denominator ; time_passed -= sample_denominator ) {
        sample_buffer.write< u8 >( level ? 0xFF : 0x00 ) ;
    }

    // Finally, accumulate the remainer for the next sample.

    sample_duration += time_passed ;
    if ( level ) {
        sample_value += time_passed ;
    }
}

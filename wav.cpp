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
 * Buffer used for holding the samples.
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
 * Level and duration accumulated so far of pulse being currently output.
 */
//@{
uint last_duration ;
bool last_level ;
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
 * Append pulse of given duration and given pulse level to WAV pulse block.
 */
void wav_out( const uint duration, const bool level )
{
    // Zero duration doesn't extend anything.

    if ( duration == 0 ) {
        return ;
    }

#if 0

    // In case the level has changed, output the previously accumulated
    // duration and prepare for the new pulse.

    if ( last_level != level ) {
        wav_pulse( last_duration ) ;
        last_duration = 0 ;
        last_level = level ;
    }

    // Extend the current pulse.

    last_duration += duration ;

    // In case the pulse duration exceeds the limit the WAV pulse encoding
    // can handle at maximum, use zero pulse to concatenate multiple pulses
    // to create pulse of required duration.

    if ( last_duration > limit ) {
        wav_pulse( limit ) ;
        wav_pulse( 0 ) ;
        last_duration -= limit ;
    }

#endif
}

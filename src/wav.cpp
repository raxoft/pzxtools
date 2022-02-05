// $Id: wav.cpp 336 2007-07-30 19:50:57Z patrik $

/**
 * @file Rendering to WAV files.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

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
uint sample_value ;
uint sample_duration ;
//@}

}

/**
 * Append pulse of given duration and given pulse level to WAV output.
 */
void wav_out( const uint duration, const bool level )
{
    // Compute how much time has passed and how much is there left
    // until the next sample starts.

    uquad time_passed = ( uquad( duration ) * sample_numerator ) ;
    const uint time_left = ( sample_denominator - sample_duration ) ;

    // If we have finished current sample, output it now.

    if ( time_passed >= time_left ) {

        // Adjust the values.

        time_passed -= time_left ;
        if ( level ) {
            sample_value += time_left ;
        }

        // Output the sample.

        sample_buffer.write< u8 >( 255ull * sample_value / sample_denominator ) ;

        // Prepare for next sample.

        sample_value = 0 ;
        sample_duration = 0 ;
    }

    // In case the time passed covered several more samples as well,
    // generate them now.

    for ( ; time_passed >= sample_denominator ; time_passed -= sample_denominator ) {
        sample_buffer.write< u8 >( level ? 255 : 0 ) ;
    }

    // Finally, accumulate the remainer for the next sample.

    sample_duration += uint( time_passed ) ;
    if ( level ) {
        sample_value += uint( time_passed ) ;
    }
}

/**
 * Flush the remaining sample to the sample buffer.
 */
void wav_flush( void )
{
    // Store the remaining sample.

    if ( sample_duration > 0 ) {
        sample_buffer.write< u8 >( 255ull * sample_value / sample_denominator ) ;

        sample_value = 0 ;
        sample_duration = 0 ;
    }
}

/**
 * Write given memory block of given size to output file.
 */
void wav_write( const void * const data, const uint size )
{
    hope( data || size == 0 ) ;
    hope( output_file ) ;

    // Just write everything, freaking out in case of problems.

    if ( std::fwrite( data, 1, size, output_file ) != size ) {
        fail( "error writing to file" ) ;
    }
}

/**
 * Write content of given buffer to output file.
 *
 * @note The buffer content is cleared afterwards, making it ready for reuse.
 */
void wav_write( Buffer & buffer )
{
    // Write entire buffer to the file.

    wav_write( buffer.get_data(), buffer.get_data_size() ) ;

    // Clear the buffer so it can be reused right away.

    buffer.clear() ;
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
 * Write everything to WAV output file and stop using that file.
 */
void wav_close( void )
{
    hope( output_file ) ;

    // Flush everything to the sample buffer.

    wav_flush() ;

    // Make sure the buffer size is even.

    uint size = sample_buffer.get_data_size() ;
    if ( ( size & 1 ) != 0 ) {
        sample_buffer.write< u8 >( 0 ) ;
        size++ ;
    }

    // Prepare the header.

    Buffer header ;

    header.write< u32 >( WAV_HEADER ) ;
    header.write_little< u32 >( 4 + ( 8 + 16 ) + ( 8 + size ) ) ;
    header.write< u32 >( WAV_WAVE ) ;

    // Continue with format chunk.

    header.write< u32 >( WAV_FORMAT ) ;
    header.write_little< u32 >( 16 ) ;
    header.write_little< u16 >( 1 ) ;                   // PCM format.
    header.write_little< u16 >( 1 ) ;                   // 1 channel.
    header.write_little< u32 >( sample_numerator ) ;    // sample rate.
    header.write_little< u32 >( sample_numerator ) ;    // byte rate.
    header.write_little< u16 >( 1 ) ;                   // block alignment.
    header.write_little< u16 >( 8 ) ;                   // bits per sample.

    // Append the header of the data chunk.

    header.write< u32 >( WAV_DATA ) ;
    header.write_little< u32 >( size ) ;

    // Now write both the header and the data to the output file.

    wav_write( header ) ;
    wav_write( sample_buffer ) ;

    // Forget about the file.

    output_file = NULL ;
}

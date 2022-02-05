// $Id: pzx2wav.cpp 359 2007-08-21 06:45:06Z patrik $

/**
 * @file PZX->WAV convertor.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#include "pzx.h"
#include "wav.h"

/**
 * Global options.
 */
namespace {

/**
 * Default sample rate for WAV generation.
 */
const uint default_sample_rate = 44100 ;

/**
 * Sample rate used for WAV generation.
 */
uint option_sample_rate = 0 ;

} ;

/**
 * Fetch value of specified type from given data block.
 */
template< typename Type >
Type fetch( const byte * & data, uint & data_size )
{
    hope( data ) ;

    if ( sizeof( Type ) > data_size ) {
        fail( "incomplete block detected" ) ;
    }

    const Type value = little_endian( * reinterpret_cast< const Type * >( data ) ) ;

    data += sizeof( Type ) ;
    data_size -= sizeof( Type ) ;

    return value ;
}

/**
 * Skip given amount of bytes in given data block.
 */
void skip( const uint amount, const byte * & data, uint & data_size )
{
    hope( data ) ;

    if ( amount > data_size ) {
        fail( "incomplete block detected" ) ;
    }

    data += amount ;
    data_size -= amount ;
}

/**
 * Macros for convenient fetching of values from current block.
 */
//@{
#define GET1()  fetch< u8 >( data, data_size )
#define GET2()  fetch< u16 >( data, data_size )
#define GET4()  fetch< u32 >( data, data_size )
#define SKIP(n) skip( n, data, data_size )
//@}

/**
 * Render bits from given byte using given (little endian) pulse sequences.
 */
void render_bits(
    bool & level,
    uint bit_count,
    uint bits,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const byte * const sequence_0,
    const byte * const sequence_1
)
{
    hope( sequence_0 || pulse_count_0 == 0 ) ;
    hope( sequence_1 || pulse_count_1 == 0 ) ;

    // Output all bits.

    while ( bit_count-- > 0 ) {

        // Choose the appropriate sequence for given bit.

        const byte * sequence ;
        uint count ;

        if ( ( bits & 0x80 ) == 0 ) {
            sequence = sequence_0 ;
            count = pulse_count_0 ;
        }
        else {
            sequence = sequence_1 ;
            count = pulse_count_1 ;
        }

        // Use next bit next time.

        bits <<= 1 ;

        // Now output the appropriate amount of pulses.

        while ( count-- > 0 ) {
            uint duration = *sequence++ ;
            duration += *sequence++ << 8 ;
            wav_out( duration, level ) ;
            level = ! level ;
        }
    }
}

/**
 * Render given DATA block to WAV output file.
 */
void render_data_block( const byte * data, uint data_size )
{
    hope( data ) ;

    // Fetch the numbers.

    uint bit_count = GET4() ;
    const uint tail_cycles = GET2() ;
    const uint pulse_count_0 = GET1() ;
    const uint pulse_count_1 = GET1() ;

    // Extract initial pulse level.

    bool level = ( ( bit_count >> 31 ) != 0 ) ;

    bit_count &= 0x7FFFFFFF ;

    // Fetch the sequences. Note that we keep them little endian here.

    const byte * const sequence_0 = data ;
    SKIP( 2 * pulse_count_0 ) ;

    const byte * const sequence_1 = data ;
    SKIP( 2 * pulse_count_1 ) ;

    // Make sure the bit count matches the block size.

    if ( data_size != ( ( bit_count + 7 ) / 8 ) ) {
        fail( "bit count %u does not match the actual data size %u", bit_count, data_size ) ;
    }

    // Now output all the bits.

    while ( bit_count > 8 ) {
        render_bits( level, 8, *data++, pulse_count_0, pulse_count_1, sequence_0, sequence_1 ) ;
        bit_count -= 8 ;
    }
    render_bits( level, bit_count, *data, pulse_count_0, pulse_count_1, sequence_0, sequence_1 ) ;

    // And finally output the optional tail pulse.

    wav_out( tail_cycles, level ) ;
}

/**
 * Render given PULSE block to WAV output file.
 */
void render_pulse_block( const byte * data, uint data_size )
{
    hope( data ) ;

    // Prepare initial level.

    bool level = false ;

    // Render all pulses in the block.

    while ( data_size > 0 ) {

        // Fetch the pulse repeat count and duration.

        uint count = 1 ;
        uint duration = GET2() ;
        if ( duration > 0x8000 ) {
            count = duration & 0x7FFF ;
            duration = GET2() ;
        }
        if ( duration >= 0x8000 ) {
            duration &= 0x7FFF ;
            duration <<= 16 ;
            duration |= GET2() ;
        }

        // Output the appropriate number of pulses.

        while ( count-- > 0 ) {
            wav_out( duration, level ) ;
            level = ! level ;
        }
    }
}

/**
 * Render given PZX block to WAV output file.
 */
void render_block( const uint tag, const byte * data, uint data_size )
{
    hope( data ) ;

    switch ( tag ) {
        case PZX_HEADER: {
            const uint major = GET1() ;
            const uint minor = GET1() ;
            if ( major != PZX_MAJOR ) {
                fail( "unsupported PZX major version %u.%u - stopping", major, minor ) ;
            }
            if ( minor > PZX_MINOR ) {
                warn( "unsupported PZX minor version %u.%u - proceeding", major, minor ) ;
            }
            break ;
        }
        case PZX_PULSES: {
            render_pulse_block( data, data_size ) ;
            break ;
        }
        case PZX_DATA: {
            render_data_block( data, data_size ) ;
            break ;
        }
        case PZX_PAUSE: {
            const uint duration = GET4() ;
            wav_out( ( duration & 0x7FFFFFFF ), ( duration >> 31 ) ) ;
            break ;
        }
    }
}

/**
 * Convert given PZX file to PZX text render.
 */
extern "C"
int main( int argc, char * * argv )
{
    // Make sure the standard I/O is in binary mode.

    set_binary_mode( stdin ) ;
    set_binary_mode( stdout ) ;

    // Parse the command line.

    const char * input_name = NULL ;
    const char * output_name = NULL ;

    for ( int i = 1 ; i < argc ; i++ ) {
        if ( argv[ i ][ 0 ] != '-' ) {
            if ( input_name ) {
                fail( "multiple input file names specified" ) ;
            }
            input_name = argv[ i ] ;
            continue ;
        }
        switch ( argv[ i ][ 1 ] ) {
            case 'o': {
                if ( output_name ) {
                    fail( "multiple output file names specified" ) ;
                }
                output_name = argv[ ++i ] ;
                break ;
            }
            case 's': {
                const char * const arg = argv[ ++i ] ;
                if ( arg == NULL ) {
                    fail( "missing sample rate" ) ;
                }
                option_sample_rate = uint( atoi( arg ) ) ;
                break ;
            }
            default: {
                fprintf( stderr, "error: invalid option %s\n", argv[ i ] ) ;

                // Fall through.
            }
            case 'h': {
                fprintf( stderr, "usage: pzx2wav [-s n] [-o output_file] [input_file]\n" ) ;
                fprintf( stderr, "-o f   write output to given file instead of standard output\n" ) ;
                fprintf( stderr, "-s n   use given sample rate instead of default %uHz\n", default_sample_rate ) ;
                return EXIT_FAILURE ;
            }
        }
    }

    // Open the input file.

    FILE * const input_file = ( input_name ? fopen( input_name, "rb" ) : stdin ) ;
    if ( input_file == NULL ) {
        fail( "unable to open input file" ) ;
    }

    // Read in the header.

    Buffer buffer ;
    if ( buffer.read( input_file, 8 ) != 8 ) {
        fail( "error reading input file" ) ;
    }

    // Make sure it is really the PZX file.

    const u32 * header = buffer.get_typed_data< u32 >() ;

    if ( header[ 0 ] != PZX_HEADER ) {
        fail( "input is not a PZX file" ) ;
    }

    // Only then open the output file.

    FILE * const output_file = ( output_name ? fopen( output_name, "wb" ) : stdout ) ;
    if ( output_file == NULL ) {
        fail( "unable to open output file" ) ;
    }

    // Bind the WAV stream to the output file.

    const uint sample_rate = ( option_sample_rate > 0 ? option_sample_rate : default_sample_rate ) ;

    wav_open( output_file, sample_rate, 3500000 ) ;

    // Now keep reading the blocks and process each one in turn.

    for ( ; ; ) {

        // Extract the tag and size from the header.

        const uint tag = native_endian( header[ 0 ] ) ;
        const uint size = little_endian( header[ 1 ] ) ;

        // Read in the block data.

        if ( buffer.read( input_file, size ) != size ) {
            fail( "error reading block data" ) ;
        }

        // Render the block.

        render_block( tag, buffer.get_data(), size ) ;

        // Read in header of the next block, if there is any.

        const uint bytes_read = buffer.read( input_file, 8 ) ;
        header = buffer.get_typed_data< u32 >() ;

        // Stop if there is nothing more.

        if ( bytes_read == 0 ) {
            break ;
        }

        // Check for errors.

        if ( bytes_read != 8 ) {
            fail( "error reading block header" ) ;
        }
    }

    // Close the input file.

    fclose( input_file ) ;

    // Finally, close the WAV stream and make sure there were no errors.

    wav_close() ;

    if ( ferror( output_file ) != 0 || fclose( output_file ) != 0 ) {
        fail( "error while closing the output file" ) ;
    }

    return EXIT_SUCCESS ;
}

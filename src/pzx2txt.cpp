// $Id: pzx2txt.cpp 1489 2011-04-21 19:07:40Z patrik $

/**
 * @file PZX->TXT convertor.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#include "pzx.h"

/**
 * Global options.
 */
namespace {

/**
 * Flag set when data should be dumped as pulses.
 */
bool option_dump_pulses ;

/**
 * Flag set when data should be dumped as ASCII characters when possible.
 */
bool option_dump_ascii ;

/**
 * Flag set when data should be dumped as headers when possible.
 */
bool option_dump_headers ;

/**
 * Flag set when dumping of content of DATA blocks should be suppresed.
 */
bool option_skip_data ;

/**
 * Flag set when each pulse should be printed on separate line.
 */
bool option_expand_pulses ;

/**
 * Flag set when initial level of each pulse should be printed as well.
 */
bool option_annotate_pulses ;

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
 * Dump single string to a file.
 */
void dump_string( FILE * const output_file, const char * const prefix, const byte * const data, const uint data_size )
{
    hope( prefix ) ;
    hope( data ) ;

    fprintf( output_file, "%s \"", prefix ) ;

    for ( uint i = 0 ; i < data_size ; i++ ) {
        const byte b = data[ i ] ;

        // Escape special characters.

        switch ( b ) {
            case '\\':
            case '"':
            {
                fprintf( output_file, "\\%c", b ) ;
                continue ;
            }
            case '\n':
            {
                fprintf( output_file, "\\n" ) ;
                continue ;
            }
            case '\r':
            {
                fprintf( output_file, "\\r" ) ;
                continue ;
            }
            case '\t':
            {
                fprintf( output_file, "\\t" ) ;
                continue ;
            }
        }

        // Any other control characters are printed in hex.

        if ( b < 32 ) {
            fprintf( output_file, "\\x%02X", b ) ;
            continue ;
        }

        // Anything else is printed verbatim. Note that this includes any characters
        // greater than 127, as the strings are supposed to be in UTF-8 encoding.

        fprintf( output_file, "%c", b ) ;
    }

    fprintf( output_file, "\"\n" ) ;
}

/**
 * Dump strings separated with null characters to a file.
 */
void dump_strings( FILE * const output_file, const char * const prefix, const byte * data, uint data_size )
{
    hope( data ) ;

    const byte * const end = data + data_size ;
    while ( data < end ) {
        const byte * const string = data ;
        while ( data < end && *data != 0 ) {
            data++ ;
        }
        dump_string( output_file, prefix, string, data - string ) ;
        data++ ;
    }
}

/**
 * Dump single data line to a file.
 */
void dump_data_line( FILE * const output_file, const byte * const data, const uint data_size, const bool dump_ascii = false )
{
    hope( data ) ;

    if ( data_size == 0 ) {
        return ;
    }

    fprintf( output_file, "BODY " ) ;

    for ( uint i = 0 ; i < data_size ; i++ ) {
        const byte b = data[ i ] ;
        if ( dump_ascii && b > 32 && b < 127 ) {
            fprintf( output_file, ".%c", b ) ;
        }
        else {
            fprintf( output_file, "%02X", b ) ;
        }
    }

    fprintf( output_file, "\n" ) ;
}

/**
 * Dump data block to a file.
 */
void dump_data( FILE * const output_file, const byte * data, uint data_size, const bool dump_ascii = false )
{
    hope( data ) ;

    if ( option_skip_data ) {
        return ;
    }

    const uint limit = 32 ;
    while ( data_size > limit ) {
        dump_data_line( output_file, data, limit, dump_ascii ) ;
        data += limit ;
        data_size -= limit ;
    }
    dump_data_line( output_file, data, data_size, dump_ascii ) ;
}

/**
 * Dump given amount of pulses of given duration to a file, toggling level as appropriate.
 */
void dump_pulses(
    FILE * const output_file,
    bool & level,
    const uint duration,
    uint count = 1
)
{
    // Output the appropriate number of pulses, according to the command line options.

    if ( option_expand_pulses ) {
        while ( count-- > 0 ) {
            if ( option_annotate_pulses ) {
              fprintf( output_file, "PULSE%u %u\n", uint( level ), duration ) ;
            }
            else {
              fprintf( output_file, "PULSE %u\n", duration ) ;
            }
            level = ! level ;
        }
        return ;
    }

    if ( option_annotate_pulses ) {
      fprintf( output_file, "PULSE%u %u", uint( level ), duration ) ;
    }
    else {
      fprintf( output_file, "PULSE %u", duration ) ;
    }
    if ( count > 1 ) {
        fprintf( output_file, " %u", count ) ;
    }
    fprintf( output_file, "\n" ) ;
    level ^= ( count & 1 ) ;
}

/**
 * Dump bits from given byte using given (little endian) pulse sequences.
 */
void dump_bits(
    FILE * const output_file,
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
            dump_pulses( output_file, level, duration ) ;
        }
    }
}

/**
 * Dump given bit sequence to given output file.
 */
void dump_bit_sequence( FILE * const output_file, const uint index, const byte * sequence, uint count )
{
    hope( sequence ) ;

    fprintf( output_file, "BIT%u", index ) ;

    while ( count-- > 0 ) {
        uint duration = *sequence++ ;
        duration += *sequence++ << 8 ;
        fprintf( output_file, " %u", duration ) ;
    }

    fprintf( output_file, "\n" ) ;
}

/**
 * Dump the DATA block to given file.
 */
void dump_data_block( FILE * const output_file, const byte * data, uint data_size )
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

    // Dump everything as pulses if requested.

    if ( option_dump_pulses ) {

        fprintf( output_file, "PULSES\n" ) ;

        // Make sure the level is high by using zero pulse if necessary.

        if ( level ) {
            level = false ;
            dump_pulses( output_file, level, 0 ) ;
        }

        // Dump all the bits.

        while ( bit_count > 8 ) {
            dump_bits( output_file, level, 8, *data++, pulse_count_0, pulse_count_1, sequence_0, sequence_1 ) ;
            bit_count -= 8 ;
        }
        dump_bits( output_file, level, bit_count, *data, pulse_count_0, pulse_count_1, sequence_0, sequence_1 ) ;

        // Include the tail pulse if necessary.

        if ( tail_cycles > 0 ) {
            dump_pulses( output_file, level, tail_cycles ) ;
        }

        return ;
    }

    // Otherwise print the data normally, staring with all the header info.

    fprintf( output_file, "DATA %u\n", level ) ;

    fprintf( output_file, "SIZE %u\n", ( bit_count / 8 ) ) ;
    if ( ( bit_count & 7 ) != 0 ) {
        fprintf( output_file, "BITS %u\n", ( bit_count & 7 ) ) ;
    }

    fprintf( output_file, "TAIL %u\n", tail_cycles ) ;

    // Now dump the bit sequences used.

    dump_bit_sequence( output_file, 0, sequence_0, pulse_count_0 ) ;
    dump_bit_sequence( output_file, 1, sequence_1, pulse_count_1 ) ;

    // If header dumping is enabled, dump whatever looks like a header in a more readable form.

    if ( option_dump_headers && data_size == 19 ) {

        const uint leader = GET1() ;
        const uint type = GET1() ;
        fprintf( output_file, "BYTE %u %u\n", leader, type ) ;

        dump_data_line( output_file, data, 10, true ) ;
        data += 10 ;

        const uint size = GET2() ;
        const uint start = GET2() ;
        const uint extra = GET2() ;
        fprintf( output_file, "WORD %u %u %u\n", size, start, extra ) ;

        const uint checksum = GET1() ;
        fprintf( output_file, "BYTE %u\n", checksum ) ;
        return ;
    }

    // Otherwise dump the data as they are.

    dump_data( output_file, data, data_size, option_dump_ascii ) ;
}

/**
 * Dump the PULSE block to given file.
 */
void dump_pulse_block( FILE * const output_file, const byte * data, uint data_size )
{
    hope( data ) ;

    fprintf( output_file, "PULSES\n" ) ;

    bool level = 0 ;

    // Dump all pulses in the block.

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

        // Output the appropriate number of pulses, according to the command line options.

        dump_pulses( output_file, level, duration, count ) ;
    }
}

/**
 * Dump given PZX block to given file.
 */
void dump_block( FILE * const output_file, const uint tag, const byte * data, uint data_size )
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
            fprintf( output_file, "PZX %u.%u\n", major, minor ) ;
            dump_strings( output_file, "INFO", data, data_size ) ;
            return ;
        }
        case PZX_PULSES: {
            dump_pulse_block( output_file, data, data_size ) ;
            return ;
        }
        case PZX_DATA: {
            dump_data_block( output_file, data, data_size ) ;
            return ;
        }
        case PZX_PAUSE: {
            const uint duration = GET4() ;
            fprintf( output_file, "PAUSE %u %u\n", ( duration & 0x7FFFFFFF ), ( duration >> 31 ) ) ;
            break ;
        }
        case PZX_STOP: {
            const uint flags = GET2() ;
            fprintf( output_file, "STOP %u\n", flags ) ;
            break ;
        }
        case PZX_BROWSE: {
            dump_string( output_file, "BROWSE", data, data_size ) ;
            return ;
        }
        default: {
            fprintf( output_file, "TAG " ) ;
            fwrite( &tag, 4, 1, output_file ) ;
            fprintf( output_file, "\n" ) ;
            fprintf( output_file, "SIZE %u\n", data_size ) ;
            dump_data( output_file, data, data_size, option_dump_ascii ) ;
            return ;
        }
    }

    if ( data_size > 0 ) {
        warn( "excessive data (%u byte%s) detected at end of block", data_size, ( data_size > 1 ? "s" : "" ) ) ;
    }
}

/**
 * Convert given PZX file to PZX text dump.
 */
extern "C"
int main( int argc, char * * argv )
{
    // Make sure the standard input is in binary mode.

    set_binary_mode( stdin ) ;

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
            case 'p': {
                option_dump_pulses = true ;
                break ;
            }
            case 'a': {
                option_dump_ascii = true ;
                break ;
            }
            case 'x': {
                option_dump_headers = true ;
                break ;
            }
            case 'd': {
                option_skip_data = true ;
                break ;
            }
            case 'e': {
                option_expand_pulses = true ;
                break ;
            }
            case 'l': {
                option_annotate_pulses = true ;
                break ;
            }
            default: {
                fprintf( stderr, "error: invalid option %s\n", argv[ i ] ) ;

                // Fall through.
            }
            case 'h': {
                fprintf( stderr, "usage: pzx2txt [-p|-a|-x|-d|-e] [-o output_file] [input_file]\n" ) ;
                fprintf( stderr, "-o f   write output to given file instead of standard output\n" ) ;
                fprintf( stderr, "-p     dump bytes in data blocks as pulses\n" ) ;
                fprintf( stderr, "-a     dump bytes in data blocks as ASCII characters when possible\n" ) ;
                fprintf( stderr, "-x     dump bytes in data blocks as headers when possible\n" ) ;
                fprintf( stderr, "-d     don't dump content of data blocks\n" ) ;
                fprintf( stderr, "-e     expand pulses, dumping each one on separate line\n" ) ;
                fprintf( stderr, "-l     print initial level of each pulse dumped\n" ) ;
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

    FILE * const output_file = ( output_name ? fopen( output_name, "w" ) : stdout ) ;
    if ( output_file == NULL ) {
        fail( "unable to open output file" ) ;
    }

    // Now keep reading the blocks and process each one in turn.

    for ( ; ; ) {

        // Extract the tag and size from the header.

        const uint tag = native_endian( header[ 0 ] ) ;
        const uint size = little_endian( header[ 1 ] ) ;

        // Read in the block data.

        if ( buffer.read( input_file, size ) != size ) {
            fail( "error reading block data" ) ;
        }

        // Dump the block.

        dump_block( output_file, tag, buffer.get_data(), size ) ;

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

        // Separate blocks with empty line.

        fprintf( output_file, "\n" ) ;
    }

    // Close both input and output files and make sure there were no errors.

    fclose( input_file ) ;

    if ( ferror( output_file ) != 0 || fclose( output_file ) != 0 ) {
        fail( "error while closing the output file" ) ;
    }

    return EXIT_SUCCESS ;
}

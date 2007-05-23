// $Id$

// PZX->TXT convertor.

#include "pzx.h"

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
 * Macros for convenient fetching of values from current block.
 */
//@{
#define GET1()  fetch< u8 >( data, data_size )
#define GET2()  fetch< u16 >( data, data_size )
#define GET4()  fetch< u32 >( data, data_size )
//@}

void dump_string( const char * const prefix, const byte * const data, const uint data_size )
{

}

void dump_strings( const char * const prefix,  const byte * const data, const uint data_size )
{

}

void dump_data( const byte * const data, const uint data_size )
{

}

/**
 * Dump given PZX block to given file.
 */
void dump_block( FILE * const output_file, const uint tag, const byte * data, uint data_size )
{
    hope( data ) ;

    // Output block name.

    switch ( tag ) {
        case PZX_HEADER: {
            const uint major = GET1() ;
            const uint minor = GET1() ;
            fprintf( output_file, "PZX %u.%u\n", major, minor ) ;
            dump_strings( "INFO", data, data_size ) ;
            break ;
        }
        case PZX_PULSES: {
            while ( data_size > 0 ) {
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
                fprintf( output_file, "PULSE %u", duration ) ;
                if ( count > 1 ) {
                    fprintf( output_file, " %u", count ) ;
                }
                fprintf( output_file, "\n" ) ;
            }
            break ;
        }
        case PZX_DATA: {
            uint bit_count = GET4() ;

            fprintf( output_file, "DATA %u\n", ( bit_count >> 31 ) ) ;
            bit_count &= 0x7FFFFFF ;

            fprintf( output_file, "SIZE %u", ( bit_count / 8 ) ) ;
            if ( ( bit_count & 7 ) != 0 ) {
                fprintf( output_file, " %u", ( bit_count &7 ) ) ;
            }
            fprintf( output_file, "\n" ) ;

            break ;
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
            dump_string( "BROWSE", data, data_size ) ;
            break ;
        }
        default: {
            fprintf( output_file, "TAG " ) ;
            fwrite( &tag, 4, 1, output_file ) ;
            fprintf( output_file, "\n" ) ;
            dump_data( data, data_size ) ;
            return ;
        }
    }
}

/**
 * Convert given PZX file to TXT dump.
 */
extern "C"
int main( int argc, char * * argv )
{
    // Make sure the standard input is in binary mode.

#ifdef _MSC_VER
    _setmode( _fileno( stdin ), _O_BINARY ) ;
#endif

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
            default: {
                fail( "invalid option %s", argv[ i ] ) ;
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

        switch ( buffer.read( input_file, 8 ) ) {
            case 8: {
                header = buffer.get_typed_data< u32 >() ;
                continue ;
            }
            case 0: {
                break ;
            }
            default: {
                fail( "error reading block header" ) ;
            }
        }

        break ;
    }

    // Close both input and output files and make sure there were no errors.

    fclose( input_file ) ;

    if ( ferror( output_file ) != 0 || fclose( output_file ) != 0 ) {
        fail( "error while closing the output file" ) ;
    }

    return EXIT_SUCCESS ;
}
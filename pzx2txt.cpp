// $Id$

// PZX->TXT convertor.

#include "pzx.h"

/**
 * Dump given PZX block to given file.
 */
void dump_block( FILE * const output_file, const uint tag, const void * data, const uint data_size )
{
    hope( data ) ;

    // Output block name.

    std::fprintf( output_file, "TAG " ) ;
    std::fwrite( &tag, 4, 1, output_file ) ;
    std::fprintf( output_file, "\n" ) ;

    switch ( tag ) {
        case PZX_HEADER:
        case PZX_PULSES:
        case PZX_DATA:
        case PZX_PAUSE:
        case PZX_STOP:
        case PZX_BROWSE:
        default:
            return ;
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

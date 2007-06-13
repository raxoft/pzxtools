// $Id$

// TAP->PZX convertor.

#include "pzx.h"
#include "tap.h"

/**
 * Global options.
 */
namespace {

/**
 * Duration of pauses inserted between tap blocks.
 */
uint option_pause_duration ;

} ;

/**
 * Convert given TAP file to PZX file.
 */
extern "C"
int main( int argc, char * * argv )
{
    // Make sure the standard I/O is in binary mode.

#ifdef _MSC_VER
    _setmode( _fileno( stdin ), _O_BINARY ) ;
    _setmode( _fileno( stdout ), _O_BINARY ) ;
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
            case 's': {
                const uint value = uint( atoi( argv[ ++i ] ) ) ;
                if ( value > 10 * 60 * 1000 ) {
                    fail( "pause duration %ums is out of range", value ) ;
                }
                option_pause_duration = value * MILLISECOND_CYCLES ;
                break ;
            }
            default: {
                fprintf( stderr, "error: invalid option %s\n", argv[ i ] ) ;

                // Fall through.
            }
            case 'h': {
                fprintf( stderr, "usage: tap2pzx [-o output_file] [input_file]\n" ) ;
                fprintf( stderr, "-o     write output to given file instead of standard output\n" ) ;
                return EXIT_FAILURE ;
            }
        }
    }

    // Open the input file.

    FILE * const input_file = ( input_name ? fopen( input_name, "rb" ) : stdin ) ;
    if ( input_file == NULL ) {
        fail( "unable to open input file" ) ;
    }

    // Open the output file.

    FILE * const output_file = ( output_name ? fopen( output_name, "wb" ) : stdout ) ;
    if ( output_file == NULL ) {
        fail( "unable to open output file" ) ;
    }

    // Bind the PZX stream to output file.

    pzx_open( output_file ) ;

    // Now read each TAP block and output it to PZX stream.

    Buffer buffer ;

    for ( ; ; ) {

        // Read in the block header, stop if there is nothing more.

        const uint bytes_read = buffer.read( input_file, 2 ) ;

        if ( bytes_read == 0 ) {
            break ;
        }

        if ( bytes_read != 2 ) {
            fail( "error reading block header" ) ;
        }

        // Fetch the block size.

        const word * const header = buffer.get_typed_data< word >() ;
        hope( header ) ;

        const uint size = little_endian( *header ) ;

        if ( size == 0 ) {
            continue ;
        }

        // Read in the block data.

        if ( buffer.read( input_file, size ) != size ) {
            fail( "error reading block data" ) ;
        }

        // Store the block to the PZX stream.

        const byte * const data = buffer.get_data() ;
        hope( data ) ;

        const uint leader_count = ( ( *data < 128 ) ? LONG_LEADER_COUNT : SHORT_LEADER_COUNT ) ;

        pzx_store( leader_count, LEADER_CYCLES ) ;
        pzx_store( 1, SYNC_1_CYCLES ) ;
        pzx_store( 1, SYNC_2_CYCLES ) ;

        static word sequence_0[] = { BIT_0_CYCLES, BIT_0_CYCLES } ;
        static word sequence_1[] = { BIT_1_CYCLES, BIT_1_CYCLES } ;

        pzx_data( data, 8 * size, true, 2, 2, sequence_0, sequence_1, TAIL_CYCLES ) ;

        // Separate the blocks with specified pause if necessary.

        if ( option_pause_duration > 0 ) {
            pzx_pause( option_pause_duration, false ) ;
        }
    }

    // Close the input file.

    fclose( input_file ) ;

    // Finally, close the PZX stream and make sure there were no errors.

    pzx_close() ;

    if ( ferror( output_file ) != 0 || fclose( output_file ) != 0 ) {
        fail( "error while closing the output file" ) ;
    }

    return EXIT_SUCCESS ;
}

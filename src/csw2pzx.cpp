// $Id: csw2pzx.cpp 359 2007-08-21 06:45:06Z patrik $

/**
 * @file CSW->PZX convertor.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#include "pzx.h"
#include "csw.h"

/**
 * Convert given CSW file to PZX file.
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
            default: {
                fprintf( stderr, "error: invalid option %s\n", argv[ i ] ) ;

                // Fall through.
            }
            case 'h': {
                fprintf( stderr, "usage: csw2pzx [-o output_file] [input_file]\n" ) ;
                fprintf( stderr, "-o f   write output to given file instead of standard output\n" ) ;
                return EXIT_FAILURE ;
            }
        }
    }

    // Now read in the input file.

    FILE * const input_file = ( input_name ? fopen( input_name, "rb" ) : stdin ) ;
    if ( input_file == NULL ) {
        fail( "unable to open input file" ) ;
    }

    Buffer buffer( 256 * 1024 ) ;

    if ( ! buffer.read( input_file ) ) {
        fail( "error reading input file" ) ;
    }

    fclose( input_file ) ;

    // Make sure it is the CSW file.

    if ( buffer.get_data_size() < 32 || std::memcmp( buffer.get_data(), "Compressed Square Wave\x1a", 23 ) != 0 ) {
        fail( "input is not a CSW file" ) ;
    }

    // Open the output file.

    FILE * const output_file = ( output_name ? fopen( output_name, "wb" ) : stdout ) ;
    if ( output_file == NULL ) {
        fail( "unable to open output file" ) ;
    }

    // Bind the PZX stream to output file.

    pzx_open( output_file ) ;

    // Now let the CSW renderer render the output to PZX stream.

    csw_render( buffer.get_data(), buffer.get_data_size() ) ;

    // Finally, close the PZX stream and make sure there were no errors.

    pzx_close() ;

    if ( ferror( output_file ) != 0 || fclose( output_file ) != 0 ) {
        fail( "error while closing the output file" ) ;
    }

    return EXIT_SUCCESS ;
}

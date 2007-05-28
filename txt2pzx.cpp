// $Id$

// TXT->PZX convertor.

#include "pzx.h"

namespace {

#define TAG_NAME(a,b,c,d)   ((a)<<24|(b)<<16|(c)<<8|(d)|0x20202020)

const uint TAG_HEADER       = TAG_NAME('P','Z','X',' ') ;
const uint TAG_INFO         = TAG_NAME('I','N','F','O') ;
const uint TAG_PULSE        = TAG_NAME('P','U','L','S') ;
const uint TAG_DATA         = TAG_NAME('D','A','T','A') ;
const uint TAG_SIZE         = TAG_NAME('S','I','Z','E') ;
const uint TAG_BITS         = TAG_NAME('B','I','T','S') ;
const uint TAG_BIT0         = TAG_NAME('B','I','T','0') ;
const uint TAG_BIT1         = TAG_NAME('B','I','T','1') ;
const uint TAG_TAIL         = TAG_NAME('T','A','I','L') ;
const uint TAG_BODY         = TAG_NAME('B','O','D','Y') ;
const uint TAG_PAUSE        = TAG_NAME('P','A','U','S') ;
const uint TAG_STOP         = TAG_NAME('S','T','O','P') ;
const uint TAG_BROWSE       = TAG_NAME('B','R','O','W') ;

} ;

/**
 * Parse string argument.
 */
const char * parse_string( const char * const string )
{
    hope( string ) ;

    // FIXME

    return string ;
}

/**
 * Process single PZX text dump line.
 */
void process_line( const char * const line )
{
    hope( line ) ;
    hope( *line ) ;

    // Fetch the tag at line start.

    const char * s = line ;

    uint tag = *s++ ;
    tag <<= 8 ;
    tag |= *s++ ;
    tag <<= 8 ;
    tag |= *s++ ;
    tag <<= 8 ;
    tag |= *s ;

    // Make it lowercase, and convert end of line to space as well.

    tag |= 0x20202020 ;

    // Find the first argument.

    s += strcspn( s, " \t" ) ;
    s += strspn( s, " \t" ) ;

    // Process the line according to the line tag.
    //
    // Note that we don't really care what the rest of the tag is.

    switch ( tag ) {
        case TAG_HEADER: {
            break ;
        }
        case TAG_INFO:
        {
            pzx_info( parse_string( s ) ) ;
            break ;
        }
        case TAG_PULSE:
        {
            break ;
        }
        case TAG_DATA:
        {
            break ;
        }
        case TAG_SIZE:
        {
            break ;
        }
        case TAG_BITS:
        {
            break ;
        }
        case TAG_BIT0:
        {
            break ;
        }
        case TAG_BIT1:
        {
            break ;
        }
        case TAG_TAIL:
        {
            break ;
        }
        case TAG_BODY:
        {
            break ;
        }
        case TAG_PAUSE:
        {
            break ;
        }
        case TAG_STOP:
        {
            break ;
        }
        case TAG_BROWSE:
        {
            pzx_browse( parse_string( s ) ) ;
            break ;
        }
        default: {
            fail( "invalid line %s", line ) ;
        }
    }
}

/**
 * Process PZX text dump lines.
 */
void process_lines( Buffer & buffer )
{
    // Terminate the input buffer first so we can check line tags easily and
    // make it easy to detect the end at the same time.

    buffer.write< u8 >( '\n' ) ;
    buffer.write< u32 >( 0 ) ;

    // Skip byte order marker, if present. Some brain-dead editors safe
    // those even to UTF-8 encoded files, sigh.

    char * data = buffer.get_typed_data< char >() ;

    if ( ( data[ 0 ] == '\xEF' ) && ( data[ 1 ] == '\xBB' ) && ( data[ 2 ] == '\xBF' ) ) {
        data += 3 ;
    }

    // Now process line by line.

    for ( ; ; ) {

        // Skip empty lines and leading whitespace.

        data += std::strspn( data, " \t\r\n" ) ;

        // Stop if we have hit the end of file.

        if ( *data == 0 ) {
            break ;
        }

        // Remember current line.

        const char * const line = data ;

        // Find end of current line and terminate it.

        data += std::strcspn( data, "\r\n" ) ;
        *data++ = 0 ;

        // Now process it.

        process_line( line ) ;
    }
}

/**
 * Convert given PZX text dump to PZX file.
 */
extern "C"
int main( int argc, char * * argv )
{
    // Make sure the standard output is in binary mode.

#ifdef _MSC_VER
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
            default: {
                fail( "invalid option %s", argv[ i ] ) ;
            }
        }
    }

    // Now read in the input file.
    //
    // Note that we could do that line by line, but why bother.

    FILE * const input_file = ( input_name ? fopen( input_name, "r" ) : stdin ) ;
    if ( input_file == NULL ) {
        fail( "unable to open input file" ) ;
    }

    Buffer buffer( 256 * 1024 ) ;

    if ( ! buffer.read( input_file ) ) {
        fail( "error reading input file" ) ;
    }

    fclose( input_file ) ;

    // Open the output file.

    FILE * const output_file = ( output_name ? fopen( output_name, "wb" ) : stdout ) ;
    if ( output_file == NULL ) {
        fail( "unable to open output file" ) ;
    }

    // Bind the PZX stream to output file.

    pzx_open( output_file ) ;

    // Now process line by line and pass the output to the PZX stream.

    process_lines( buffer ) ;

    // Finally, close the PZX stream and make sure there were no errors.

    pzx_close() ;

    if ( ferror( output_file ) != 0 || fclose( output_file ) != 0 ) {
        fail( "error while closing the output file" ) ;
    }

    return EXIT_SUCCESS ;
}

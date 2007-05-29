// $Id$

// TXT->PZX convertor.

#include "pzx.h"

#include <cstring>
#include <cerrno>

/**
 * Global stuff.
 */
namespace {

/**
 * Line tags.
 */
//@{

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
const uint TAG_TAG          = TAG_NAME('T','A','G',' ') ;

//@}

/**
 * Buffers and variables used for collecting various information about current block.
 */
//@{

Buffer data_buffer ;
Buffer bit_0_buffer ;
Buffer bit_1_buffer ;
uint tail_cycles ;
uint expected_data_size ;
uint extra_bit_count ;
bool output_level ;
uint unknown_tag ;

//@}

/**
 * Flag set when pulse sequences should be stored exactly as they were specified.
 */
bool option_preserve_pulses ;

} ;

/**
 * Parse integral number.
 */
bool parse_number( uint & number, const char * & string, const uint maximum = 0, const char * const what = NULL )
{
    hope( string ) ;

    // Choose base ourselves, as we don't want the octal behavior.

    uint base = 10 ;

    if ( string[ 0 ] == '0' ) {
        switch ( string[ 1 ] ) {
            case 'x':
            case 'X':
            {
                base = 16 ;
                string += 2 ;
                break ;
            }
            case 'b':
            case 'B':
            {
                base = 2 ;
                string += 2 ;
                break ;
            }
        }
    }

    // Convert the number.

    errno = 0 ;
    char * end = NULL ;

    const unsigned long result = strtoul( string, &end, base ) ;

    // Deal with errors.

    if ( ( string == end ) || ( errno == ERANGE ) ) {
        if ( what ) {
            warn( "invalid %s encountered in string %s", what, string ) ;
        }
        return false ;
    }

    if ( maximum > 0 && result > maximum ) {
        warn( "%s %lu out of range in string %s", ( what ? what : "value" ), result, string ) ;
        return false ;
    }

    // Return the result otherwise.

    number = result ;

    string = end + strspn( end, " \t" ) ;

    return true ;
}

/**
 * Parse hexadecimal digit.
 */
uint parse_hex_digit( const char * & string )
{
    const char c = *string++ ;

    if ( c >= '0' && c <= '9' ) {
        return c - '0' ;
    }
    if ( c >= 'a' && c <= 'f' ) {
        return 10 + c - 'a' ;
    }
    if ( c >= 'A' && c <= 'F' ) {
        return 10 + c - 'A' ;
    }

    string-- ;

    warn( "invalid hexadecimal digit 0x%02x (%c) in string %s", c, c, string ) ;
    return 0 ;
}

/**
 * Parse hexadecimal number.
 */
uint parse_hex_number( const char * & string )
{
    uint result = parse_hex_digit( string ) ;
    result <<= 4 ;
    result |= parse_hex_digit( string ) ;
    return result ;
}

/**
 * Parse string argument.
 */
const char * parse_string( const char * const string )
{
    hope( string ) ;

    // The underlying buffer is ours anyway, so we may as well to use it for output.

    const char * in = string ;
    char * out = const_cast< char * >( in ) ;

    // Check leading quote.

    if ( *in == '"' ) {
        in++ ;
    }
    else {
        warn( "missing opening quote in a string %s", string ) ;
    }

    // Now process each character until we hit end of the string.

    for ( ; ; ) {

        char c = *in++ ;

        // Stop on quote.

        if ( c == '"' ) {
            break ;
        }

        // Process escaped characters.

        if ( c == '\\' ) {
            c = *in++ ;
            switch ( c ) {
                case 'n': {
                    c = '\n' ;
                    break ;
                }
                case 'r': {
                    c = '\r' ;
                    break ;
                }
                case 't': {
                    c = '\t' ;
                    break ;
                }
                case 'x': {
                    c = parse_hex_number( in ) ;
                    break ;
                }
            }
        }

        // Make sure the string it is properly terminated.
        //
        // Note that it misreports \x00 as well, but that is not valid in the string anyway.

        if ( c == 0 ) {
            warn( "missing closing quote in a string %s", string ) ;
            break ;
        }

        // Store it to the output.

        *out++ = c ;
    }

    // Terminate the string and return it.

    *out++ = 0 ;

    return string ;
}

/**
 * Parse string argument.
 */
void parse_data_line( const char * const string )
{
    hope( string ) ;

    // Process each character until we hit end of line.

    const char * s = string ;

    for ( ; ; ) {

        char c = *s ;

        // Stop when we hit end of line.

        if ( c == 0 ) {
            break ;
        }

        // If it is a dot-escaped ASCII char, fetch it as it is.

        if ( c == '.' ) {
            s++ ;
            c = *s++ ;

            // In case the line ends here, report assume there was a space which
            // was stripped by an editor.

            if ( c == 0 ) {
                s-- ;
                c = ' ' ;
            }
        }

        // Otherwise fetch the hex encoded value.

        else {
            c = parse_hex_number( s ) ;
        }

        // Now store it to the buffer.

        data_buffer.write< u8 >( c ) ;
    }
}

/**
 * Finish current block.
 */
void finish_block( uint & tag, const uint new_tag )
{
    // Flush the previously buffered data.

    if ( tag != 0 ) {
        pzx_flush() ;
    }

    // Process the gathered data.

    switch ( tag ) {
        case TAG_DATA:
        {
            uint data_size = data_buffer.get_data_size() ;
            if ( data_size > 0x80000000 / 8 ) {
                fail( "the data block is too big" ) ;
            }

            uint bit_count = 8 * data_size ;
            if ( extra_bit_count > 0 && bit_count > 0 ) {
                bit_count -= 8 ;
                bit_count += extra_bit_count ;
            }
            if ( bit_count == 0 ) {
                fail( "empty data block" ) ;
            }
            if ( expected_data_size > 0 && ( bit_count / 8 ) != expected_data_size ) {
                warn( "the specified byte size %u doesn't match the actual size %u", expected_data_size, bit_count / 8 ) ;
            }

            const uint pulse_count_0 = bit_0_buffer.get_data_size() / 2 ;
            const uint pulse_count_1 = bit_1_buffer.get_data_size() / 2 ;

            if ( pulse_count_0 == 0 || pulse_count_1 == 0 ) {
                fail( "unspecified bit sequence" ) ;
            }
            if ( pulse_count_0 > 0xFF || pulse_count_1 > 0xFF ) {
                fail( "too many pulses in bit sequence" ) ;
            }

            pzx_data(
                data_buffer.get_data(),
                bit_count,
                output_level,
                pulse_count_0,
                pulse_count_1,
                bit_0_buffer.get_typed_data< u16 >(),
                bit_1_buffer.get_typed_data< u16 >(),
                tail_cycles
            ) ;

            data_buffer.clear() ;
            bit_0_buffer.clear() ;
            bit_1_buffer.clear() ;

            expected_data_size = 0 ;
            extra_bit_count = 0 ;
            tail_cycles = 0 ;
            output_level = false ;

            break ;
        }
        case TAG_TAG:
        {
            const uint data_size = data_buffer.get_data_size() ;
            if ( expected_data_size > 0 && data_size != expected_data_size ) {
                warn( "the specified byte size %u doesn't match the actual size %u", expected_data_size, data_size ) ;
            }
            pzx_write_buffer( unknown_tag, data_buffer ) ;
            expected_data_size = 0 ;
            break ;
        }
    }

    // Remember new block.

    tag = new_tag ;
}


/**
 * Process single PZX text dump line.
 */
void process_line( uint & last_block_tag, const char * const line )
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
            finish_block( last_block_tag, tag ) ;
            break ;
        }
        case TAG_INFO:
        {
            pzx_info( parse_string( s ) ) ;
            break ;
        }
        case TAG_PULSE:
        {
            // Start new sequence if necessary.

            uint duration ;
            if ( ! parse_number( duration, s ) ) {
                finish_block( last_block_tag, tag ) ;
                output_level = false ;
                break ;
            }

            // Fetch the optional count, too.

            uint count = 1 ;
            parse_number( count, s ) ;

            // Store the pulse sequence as specified if requested and possible.

            if ( option_preserve_pulses ) {
                if ( count < 0x8000 && duration < 0x8000000 ) {
                    pzx_store( count, duration ) ;
                    break ;
                }
                warn( "pulse values %u*%u are out of range to be stored as they are", duration, count ) ;
            }

            // Otherwise let the output stream process each pulse as needed.

            for ( uint i = 0 ; i < count ; i++ ) {
                pzx_out( duration, output_level ) ;
                output_level = ! output_level ;
            }
            break ;
        }
        case TAG_DATA:
        {
            const uint previous_tag = last_block_tag ;

            finish_block( last_block_tag, tag ) ;

            uint level = 0 ;
            parse_number( level, s, 1, "initial pulse level" ) ;

            if ( previous_tag == TAG_PULSE && level != output_level ) {
                warn( "initial pulse level of data block is the same as the level of the last preceding pulse" ) ;
            }

            output_level = ( level != 0 ) ;

            break ;
        }
        case TAG_SIZE:
        {
            parse_number( expected_data_size, s, 0, "byte size" ) ;
            break ;
        }
        case TAG_BITS:
        {
            parse_number( extra_bit_count, s, 8, "extra bit count" ) ;
            break ;
        }
        case TAG_BIT0:
        {
            uint duration ;
            while ( parse_number( duration, s, 0xFFFF ) ) {
                bit_0_buffer.write( little_endian< u16 >( duration ) ) ;
            }
            break ;
        }
        case TAG_BIT1:
        {
            uint duration ;
            while ( parse_number( duration, s, 0xFFFF ) ) {
                bit_1_buffer.write( little_endian< u16 >( duration ) ) ;
            }
            break ;
        }
        case TAG_TAIL:
        {
            parse_number( tail_cycles, s, 0xFFFF, "tail pulse duration" ) ;
            break ;
        }
        case TAG_BODY:
        {
            parse_data_line( s ) ;
            break ;
        }
        case TAG_PAUSE:
        {
            finish_block( last_block_tag, tag ) ;

            uint duration = 1 ;
            parse_number( duration, s, 0x7FFFFFFF, "pause duration" ) ;
            uint level = 0 ;
            parse_number( level, s, 1, "pause level" ) ;

            pzx_pause( duration, level ) ;
            break ;
        }
        case TAG_STOP:
        {
            finish_block( last_block_tag, tag ) ;

            uint flags = 0 ;
            parse_number( flags, s, 0xFFFF, "stop flags" ) ;

            pzx_stop( flags ) ;
            break ;
        }
        case TAG_BROWSE:
        {
            finish_block( last_block_tag, tag ) ;
            pzx_browse( parse_string( s ) ) ;
            break ;
        }
        case TAG_TAG:
        {
            finish_block( last_block_tag, tag ) ;
            if ( strcspn( s, " \t" ) != 4 ) {
                fail( "invalid tag name %s", s ) ;
            }
            unknown_tag = PZX_TAG( s[0], s[1], s[2], s[3] ) ;
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

    uint last_block_tag = 0 ;

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

        process_line( last_block_tag, line ) ;
    }

    // Finish the last block.

    finish_block( last_block_tag, 0 ) ;
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
            case 'p': {
                option_preserve_pulses = true ;
                break ;
            }
            default: {
                fprintf( stderr, "error: invalid option %s\n", argv[ i ] ) ;

                // Fall through.
            }
            case 'h': {
                fprintf( stderr, "usage: txt2pzx [-p] [-o output_file] [input_file]\n" ) ;
                fprintf( stderr, "-o     write output to given file instead of standard output\n" ) ;
                fprintf( stderr, "-p     store pulse sequences exactly as specified\n" ) ;
                return EXIT_FAILURE ;
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

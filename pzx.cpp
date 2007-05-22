// $Id$

// Saving to PZX files.

#include "pzx.h"
#include "buffer.h"

namespace {

FILE * output_file ;

Buffer header_buffer ;

Buffer pulse_buffer ;

uint pulse_count ;
uint pulse_duration ;

uint last_duration ;
bool last_level ;

}

void pzx_open( FILE * file )
{
    hope( file ) ;

    hope( output_file == NULL ) ;
    output_file = file ;

    pzx_header( NULL, 0 ) ;
}

void pzx_close( void )
{
    hope( output_file ) ;

    pzx_flush() ;

    output_file = NULL ;
}

void pzx_write( const void * const data, const uint size )
{
    hope( output_file ) ;

    if ( std::fwrite( data, size, 1, output_file ) != 1 ) {
        fail( "error writing to file" ) ;
    }
}

void pzx_write_block( const uint tag, const void * const data, const uint size )
{
    u32 header[ 2 ] ;
    header[ 0 ] = big_endian< u32 >( tag ) ;
    header[ 1 ] = little_endian< u32 >( size ) ;

    pzx_write( header, sizeof( header ) ) ;
    pzx_write( data, size ) ;
}

void pzx_write_buffer( const uint tag, Buffer & buffer )
{
    pzx_write_block( tag, buffer.get_data(), buffer.get_data_size() ) ;
    buffer.clear() ;
}

void pzx_header( const void * const data, const uint size )
{
    hope( data || size == 0 ) ;

    if ( header_buffer.is_empty() ) {
        header_buffer.write_little< u8 >( PZX_MAJOR ) ;
        header_buffer.write_little< u8 >( PZX_MINOR ) ;
    }

    header_buffer.write( data, size ) ;
}

void pzx_info( const void * const string, const uint size )
{
    pzx_header( data, size ) ;
    header_buffer.write< u8 >( 0 ) ;
}

void pzx_info( const char * const string )
{
    hope( string ) ;
    pzx_header( string, std::strlen( string ) + 1 ) ;
}

void pzx_store( const uint count, const uint duration )
{
    hope( count > 0 ) ;
    hope( count < 0x8000 ) ;
    hope( duration < 0x80000000 ) ;

    if ( count > 1 || duration > 0xFFFF ) {
        pulse_buffer.write_little< u16 >( 0x8000 | count ) ;
    }

    if ( duration < 0x8000 ) {
        pulse_buffer.write_little< u16 >( duration ) ;
    }
    else {
        pulse_buffer.write_little< u16 >( 0x8000 | ( duration >> 16 ) ) ;
        pulse_buffer.write_little< u16 >( duration & 0xFFFF ) ;
    }
}

void pzx_pulse( const uint duration )
{
    hope( duration < 0x80000000 ) ;

    if ( pulse_count > 0 ) {

        if ( pulse_duration == duration && pulse_count < 0x7FFF ) {
            pulse_count++ ;
            return ;
        }

        pzx_store( pulse_count, pulse_duration ) ;
    }

    pulse_duration = duration ;
    pulse_count = 1 ;
}

void pzx_out( const uint duration, const bool level )
{
    hope( duration < 0x80000000 ) ;

    if ( duration == 0 ) {
        return ;
    }

    if ( last_level != level ) {
        pzx_pulse( last_duration ) ;
        last_duration = 0 ;
        last_level = level ;
    }

    last_duration += duration ;

    const uint limit = 0x7FFFFFFF ;
    if ( last_duration > limit ) {
        pzx_pulse( limit ) ;
        pzx_pulse( 0 ) ;
        last_duration -= limit ;
    }
}

void pzx_flush( void )
{
    if ( header_buffer.is_not_empty() ) {
        pzx_write_buffer( PZX_HEADER, header_buffer ) ;
    }

    if ( last_duration > 0 ) {
        pzx_pulse( last_duration ) ;
        last_duration = 0 ;
        last_level = false ;
    }

    if ( pulse_count > 0 ) {
        pzx_store( pulse_count, pulse_duration ) ;
        pulse_count = 0 ;
    }

    if ( pulse_buffer.is_not_empty() ) {
        pzx_write_buffer( PZX_PULSE, pulse_buffer ) ;
    }
}

void pzx_data(
    const byte * const data,
    const uint bit_count,
    const bool initial_level,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const u16 * const pulse_sequence_0,
    const u16 * const pulse_sequence_1,
    const uint tail_cycles
)
{
    hope( data ) ;
    hope( bit_count > 0 ) ;
    hope( bit_count < 0x80000000 ) ;
    hope( pulse_count_0 > 0 ) ;
    hope( pulse_count_0 <= 0xFF ) ;
    hope( pulse_count_1 > 0 ) ;
    hope( pulse_count_1 <= 0xFF ) ;
    hope( pulse_sequence_0 ) ;
    hope( pulse_sequence_1 ) ;
    hope( tail_count <= 0xFFFF ) ;

    pzx_flush() ;

    header_buffer.write_little< u32 >( ( initial_level << 31 ) | bit_count ) ;
    header_buffer.write_little< u16 >( tail_cycles ) ;
    header_buffer.write_little< u8 >( pulse_count_0 ) ;
    header_buffer.write_little< u8 >( pulse_count_1 ) ;

    pzx_write_buffer( PZX_DATA, header_buffer ) ;

    pzx_write( pulse_sequence_0, 2 * pulse_count_0 ) ;
    pzx_write( pulse_sequence_1, 2 * pulse_count_1 ) ;

    pzx_write( data, ( bit_count + 7 ) / 8 ) ;
}

void pzx_pause( const uint duration, const bool level )
{
    hope( duration > 0 ) ;
    hope( duration < 0x80000000 ) ;

    pzx_flush() ;
    header_buffer.write_little< u32 >( ( level << 31 ) | duration ) ;
    pzx_write_buffer( PZX_PAUSE, header_buffer ) ;
}

void pzx_stop( const uint flags )
{
    hope( flags < 0x8000 ) ;

    pzx_flush() ;
    header_buffer.write_little< u16 >( flags ) ;
    pzx_write_buffer( PZX_STOP, header_buffer ) ;
}

void pzx_browse( const void * const data, const uint size )
{
    pzx_flush() ;
    pzx_write_block( PZX_BROWSE, data, size ) ;
}

void pzx_browse( const char * const string )
{
    hope( string ) ;
    pzx_browse( string, std::strlen( string ) ) ;
}

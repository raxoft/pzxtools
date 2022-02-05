// $Id: pzx.cpp 302 2007-06-15 07:37:58Z patrik $

/**
 * @file Saving to PZX files.
 *
 * Reference implementation written in C-like style, so it should be simple
 * enough to rewrite to either object oriented or simple procedural languages.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#include "pzx.h"

namespace {

/**
 * File currently used for output, if any.
 */
FILE * output_file ;

/**
 * Buffer used for PZX header.
 */
Buffer header_buffer ;

/**
 * Buffer used for accumulating content of current PULS block.
 */
Buffer pulse_buffer ;

/**
 * Buffer used for temporary block data.
 */
Buffer data_buffer ;

/**
 * Buffer used for pulse packing.
 */
Buffer pack_buffer ;

/**
 * Count and duration of most recently stored pulses, not yet commited to the pulse buffer.
 */
//@{
uint pulse_count ;
uint pulse_duration ;
//@}

/**
 * Level and duration accumulated so far of pulse being currently output.
 */
//@{
uint last_duration ;
bool last_level ;
//@}

}

/**
 * Use given file for subsequent PZX output.
 */
void pzx_open( FILE * file )
{
    hope( file ) ;

    // Remember the file.

    hope( output_file == NULL ) ;
    output_file = file ;

    // Make sure the file starts with a PZX header.

    pzx_header( NULL, 0 ) ;
}

/**
 * Commit any buffered PZX output to PZX output file and stop using that file.
 */
void pzx_close( void )
{
    hope( output_file ) ;

    // Flush pending output.

    pzx_flush() ;

    // Forget about the file.

    output_file = NULL ;
}

/**
 * Write given memory block of given size to output file.
 */
void pzx_write( const void * const data, const uint size )
{
    hope( data || size == 0 ) ;
    hope( output_file ) ;

    // Just write everything, freaking out in case of problems.

    if ( std::fwrite( data, 1, size, output_file ) != size ) {
        fail( "error writing to file" ) ;
    }
}

/**
 * Write given memory block of given size to output file as PZX block with given tag.
 */
void pzx_write_block( const uint tag, const void * const data, const uint size )
{
    // Prepare block header.

    u32 header[ 2 ] ;
    header[ 0 ] = native_endian< u32 >( tag ) ;
    header[ 1 ] = little_endian< u32 >( size ) ;

    // Write the header followed by the data to the file.

    pzx_write( header, sizeof( header ) ) ;
    pzx_write( data, size ) ;
}

/**
 * Write content of given buffer to output file as PZX block with given tag.
 *
 * @note The buffer content is cleared afterwards, making it ready for reuse.
 */
void pzx_write_buffer( const uint tag, Buffer & buffer )
{
    // Write entire buffer to the file.

    pzx_write_block( tag, buffer.get_data(), buffer.get_data_size() ) ;

    // Clear the buffer so it can be reused right away.

    buffer.clear() ;
}

/**
 * Append given memory block of given size to PZX header block.
 */
void pzx_header( const void * const data, const uint size )
{
    hope( data || size == 0 ) ;

    // Start with the version number if the buffer is still empty.

    if ( header_buffer.is_empty() ) {
        header_buffer.write_little< u8 >( PZX_MAJOR ) ;
        header_buffer.write_little< u8 >( PZX_MINOR ) ;
    }

    // Now append the provided data.

    header_buffer.write( data, size ) ;
}


/**
 * Append given amount of characters from given string to PZX header block.
 */
void pzx_info( const void * const string, const uint length )
{
    // Separate multiple strings with zero byte.

    if ( header_buffer.get_data_size() > 2 ) {
        header_buffer.write< u8 >( 0 ) ;
    }

    // Write the string itself.

    pzx_header( string, length ) ;
}

/**
 * Append given null terminated string to PZX header block.
 */
void pzx_info( const char * const string )
{
    hope( string ) ;
    pzx_info( string, std::strlen( string ) ) ;
}

/**
 * Append given amount of pulses of given duration to PZX pulse block.
 *
 * @note The @a count and @a duration must fit in 15 and 31 bits, respectively.
 */
void pzx_store( const uint count, const uint duration )
{
    hope( count > 0 ) ;
    hope( count < 0x8000 ) ;
    hope( duration < 0x80000000 ) ;

    // Store the count if there were multiple pulses or the duration encoding requires that.

    if ( count > 1 || duration > 0xFFFF ) {
        pulse_buffer.write_little< u16 >( 0x8000 | count ) ;
    }

    // Now store the duration itself using either the short or long encoding.

    if ( duration < 0x8000 ) {
        pulse_buffer.write_little< u16 >( duration ) ;
    }
    else {
        pulse_buffer.write_little< u16 >( 0x8000 | ( duration >> 16 ) ) ;
        pulse_buffer.write_little< u16 >( duration & 0xFFFF ) ;
    }
}

/**
 * Append pulse of given duration to PZX pulse block.
 *
 * @note The @a duration must fit in 31 bits.
 */
void pzx_pulse( const uint duration )
{
    hope( duration < 0x80000000 ) ;

    // If there were already some pulses before, see if the new one
    // has the same duration as well.

    if ( pulse_count > 0 ) {

        // If the duration matches and the count limit permits, just
        // increase the repeat count.

        if ( pulse_duration == duration && pulse_count < 0x7FFF ) {
            pulse_count++ ;
            return ;
        }

        // Otherwise store the previous pulse(s) before remembering the new one.

        pzx_store( pulse_count, pulse_duration ) ;
    }

    // Remember the new pulse and its duration.

    pulse_duration = duration ;
    pulse_count = 1 ;
}

/**
 * Append pulse of given duration and given pulse level to PZX pulse block.
 */
void pzx_out( const uint duration, const bool level )
{
    // Zero duration doesn't extend anything.

    if ( duration == 0 ) {
        return ;
    }

    // In case the duration is too long that it would cause overflow
    // problems, process it as multiple pulses of same level.

    const uint limit = 0x7FFFFFFF ;

    if ( duration > limit ) {
        pzx_out( limit, level ) ;
        pzx_out( duration - limit, level ) ;
        return ;
    }

    // In case the level has changed, output the previously accumulated
    // duration and prepare for the new pulse.

    if ( last_level != level ) {
        pzx_pulse( last_duration ) ;
        last_duration = 0 ;
        last_level = level ;
    }

    // Extend the current pulse.

    last_duration += duration ;

    // In case the pulse duration exceeds the limit the PZX pulse encoding
    // can handle at maximum, use zero pulse to concatenate multiple pulses
    // to create pulse of required duration.

    if ( last_duration > limit ) {
        pzx_pulse( limit ) ;
        pzx_pulse( 0 ) ;
        last_duration -= limit ;
    }
}

/**
 * Commit any buffered header and/or pulse output to the PZX output file.
 */
void pzx_flush( void )
{
    // First goes the header, if there is any.

    if ( header_buffer.is_not_empty() ) {
        pzx_write_buffer( PZX_HEADER, header_buffer ) ;
    }

    // Then finish the last pulse, if there is any.

    if ( last_duration > 0 ) {
        pzx_pulse( last_duration ) ;
        last_duration = 0 ;
        last_level = false ;
    }

    // Now if there are some pending pulses, store them to the buffer.

    if ( pulse_count > 0 ) {
        pzx_store( pulse_count, pulse_duration ) ;
        pulse_count = 0 ;
    }

    // Finally write the entire pulse buffer to file, unless it's empty.

    if ( pulse_buffer.is_not_empty() ) {
        pzx_write_buffer( PZX_PULSES, pulse_buffer ) ;
    }
}

/**
 * Append given data block to PZX output file as PZX data block.
 */
void pzx_data(
    const byte * const data,
    const uint bit_count,
    const bool initial_level,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const word * const pulse_sequence_0,
    const word * const pulse_sequence_1,
    const uint tail_cycles
)
{
    hope( data || bit_count == 0 ) ;
    hope( bit_count < 0x80000000 ) ;
    hope( pulse_count_0 <= 0xFF ) ;
    hope( pulse_count_1 <= 0xFF ) ;
    hope( pulse_sequence_0 || pulse_count_0 == 0 ) ;
    hope( pulse_sequence_1 || pulse_count_1 == 0 ) ;
    hope( tail_cycles <= 0xFFFF ) ;

    // Flush previously buffered output.

    pzx_flush() ;

    // Prepare the header.

    data_buffer.write_little< u32 >( ( initial_level << 31 ) | bit_count ) ;
    data_buffer.write_little< u16 >( tail_cycles ) ;
    data_buffer.write_little< u8 >( pulse_count_0 ) ;
    data_buffer.write_little< u8 >( pulse_count_1 ) ;

    // Store the sequences.

    for ( uint i = 0 ; i < pulse_count_0 ; i++ ) {
        data_buffer.write_little< u16 >( pulse_sequence_0[ i ] ) ;
    }
    for ( uint i = 0 ; i < pulse_count_1 ; i++ ) {
        data_buffer.write_little< u16 >( pulse_sequence_1[ i ] ) ;
    }

    // Copy the bit stream data themselves.

    data_buffer.write( data, ( bit_count + 7 ) / 8 ) ;

    // Now write the entire block to the file.

    pzx_write_buffer( PZX_DATA, data_buffer ) ;
}

/**
 * Test if given pulse sequence matches pulses in given pulse stream.
 */
bool pzx_matches( const word * const pulses, const word * const end, const word * const sequence, const uint count )
{
    hope( pulses ) ;
    hope( end ) ;
    hope( pulses <= end ) ;
    hope( sequence || count == 0 ) ;

    if ( ( count == 0 ) || ( count > uint( end - pulses ) ) ) {
        return false ;
    }

    for ( uint i = 0 ; i < count ; i++ ) {
        if ( pulses[ i ] != sequence[ i ] ) {
            return false ;
        }
    }

    return true ;
}

/**
 * Try to pack given pulses to PZX data block using given pulse sequences.
 */
bool pzx_pack(
    const word * const pulses,
    const uint pulse_count,
    const bool initial_level,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const word * const sequence_0,
    const word * const sequence_1,
    const uint tail_cycles
)
{
    hope( pulses ) ;
    hope( pulse_count > 0 ) ;
    hope( pulse_count_0 <= 0xFF ) ;
    hope( pulse_count_1 <= 0xFF ) ;
    hope( sequence_0 || pulse_count_0 == 0 ) ;
    hope( sequence_1 || pulse_count_1 == 0 ) ;

    // Prepare for packing.

    pack_buffer.clear() ;

    const word * const end = pulses + pulse_count ;
    const word * data = pulses ;

    byte value = 0 ;
    uint bit_count = 0 ;

    // Try packing until we reach the end of the pulse stream.

    while ( data < end ) {

        // See which of the sequences matches at given position.

        if ( pzx_matches( data, end, sequence_0, pulse_count_0 ) ) {
            value <<= 1 ;
            data += pulse_count_0 ;
        }
        else if ( pzx_matches( data, end, sequence_1, pulse_count_1 ) ) {
            value <<= 1 ;
            value |= 1 ;
            data += pulse_count_1 ;
        }

        // Otherwise report failure.

        else {
            return false ;
        }

        // Include the new bit, checking the maximum limit as well.

        bit_count++ ;

        if ( bit_count >= 0x80000000 ) {
            return false ;
        }

        // Store the completed byte to the buffer when necessary.

        if ( ( bit_count & 7 ) == 0 ) {
            pack_buffer.write< byte >( value ) ;
        }
    }

    hope( data == end ) ;

    // Output the last byte, if any.

    const uint extra_bits = ( bit_count & 7 ) ;

    if ( extra_bits > 0 ) {
        value <<= ( 8 - extra_bits ) ;
        pack_buffer.write< byte >( value ) ;
    }

    // Now write the data to the DATA block.

    pzx_data(
        pack_buffer.get_data(),
        bit_count,
        initial_level,
        pulse_count_0,
        pulse_count_1,
        sequence_0,
        sequence_1,
        tail_cycles
    ) ;

    // Report success.

    return true ;
}

/**
 * Try to pack given pulses to PZX data block using given pulse sequences.
 */
bool pzx_pack(
    const word * const pulses,
    const uint pulse_count,
    const bool initial_level,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const word * const sequence_0,
    const word * const sequence_1,
    const uint sequence_order,
    const uint tail_cycles
)
{
    hope( sequence_0 || pulse_count_0 == 0 ) ;
    hope( sequence_1 || pulse_count_1 == 0 ) ;

    // Use the specified sequence order. If it is not specified explicitly,
    // try to guess it automatically, using the sequence with shorter
    // duration for bit 0.

    uint order = sequence_order ;

    if ( order > 1 ) {
        uint duration_0 = 0 ;
        for ( uint i = 0 ; i < pulse_count_0 ; i++ ) {
            duration_0 += sequence_0[ i ] ;
        }
        uint duration_1 = 0 ;
        for ( uint i = 0 ; i < pulse_count_1 ; i++ ) {
            duration_1 += sequence_1[ i ] ;
        }

        order = ( ( duration_0 == 0 ) || ( duration_1 == 0 ) || ( duration_0 <= duration_1 ) ? 0 : 1 ) ;
    }

    if ( order == 0 ) {
        return pzx_pack( pulses, pulse_count, initial_level, pulse_count_0, pulse_count_1, sequence_0, sequence_1, tail_cycles ) ;
    }
    else {
        return pzx_pack( pulses, pulse_count, initial_level, pulse_count_1, pulse_count_0, sequence_1, sequence_0, tail_cycles ) ;
    }
}

/**
 * Try to pack given pulses to PZX data block, guessing the pulse sequences automatically.
 */
bool pzx_pack(
    const word * const pulses,
    const uint pulse_count,
    const bool initial_level,
    const uint sequence_limit,
    const uint sequence_order,
    const uint tail_cycles
)
{
    hope( pulses || pulse_count == 0 ) ;

    // Make sure the limit is sane.

    uint limit = sequence_limit ;

    if ( limit > pulse_count ) {
        limit = pulse_count ;
    }

    if ( limit > 255 ) {
        limit = 255 ;
    }

    // Try all sequence combinations shorter than given limit.
    //
    // One of the sequences always starts at the beginning.

    const word * const end = pulses + pulse_count ;

    const word * const sequence_0 = pulses ;

    for ( uint pulse_count_0 = limit ; pulse_count_0 > 0 ; pulse_count_0-- ) {

        // Find where the other sequence starts.
        //
        // Note that matching fails before we try to reach behind the buffer end,
        // so we don't have to deal with that explicitly.

        const word * sequence_1 = pulses ;

        while ( pzx_matches( sequence_1, end, sequence_0, pulse_count_0 ) ) {
            sequence_1 += pulse_count_0 ;
        }

        // In the rare case the entire stream can be encoded with just one sequence, do that.

        if ( sequence_1 == end ) {
            pzx_pack( pulses, pulse_count, initial_level, pulse_count_0, 0, sequence_0, NULL, sequence_order, tail_cycles ) ;
            return true ;
        }

        // Otherwise try shortening the second sequence until the packing succeeds.
        //
        // Again, note that matching fails before we try to reach behind the buffer end,
        // so we don't have to deal with that explicitly.

        for ( uint pulse_count_1 = limit ; pulse_count_1 > 0 ; pulse_count_1-- ) {
            if ( pzx_pack( pulses, pulse_count, initial_level, pulse_count_0, pulse_count_1, sequence_0, sequence_1, sequence_order, tail_cycles ) ) {
                return true ;
            }
        }
    }

    // Report failure.

    return false ;
}

/**
 * Output given pulses to the output.
 */
void pzx_pulses(
    const word * const pulses,
    const uint pulse_count,
    const bool initial_level,
    const uint tail_cycles
)
{
    hope( pulses || pulse_count == 0 ) ;

    // Output each pulse in turn.

    bool level = initial_level ;

    for ( uint i = 0 ; i < pulse_count ; i++ ) {
        pzx_out( pulses[ i ], level ) ;
        level = ! level ;
    }

    pzx_out( tail_cycles, level ) ;
}

/**
 * Append PZX pause block of pause of given duration and level to PZX output file.
 *
 * @note The duration must fit in 31 bits.
 */
void pzx_pause( const uint duration, const bool level )
{
    hope( duration < 0x80000000 ) ;

    pzx_flush() ;
    data_buffer.write_little< u32 >( ( level << 31 ) | duration ) ;
    pzx_write_buffer( PZX_PAUSE, data_buffer ) ;
}

/**
 * Append PZX stop block with given flags to PZX output file.
 */
void pzx_stop( const uint flags )
{
    hope( flags <= 0xFFFF ) ;

    pzx_flush() ;
    data_buffer.write_little< u16 >( flags ) ;
    pzx_write_buffer( PZX_STOP, data_buffer ) ;
}

/**
 * Append PZX browse block using given amount of characters from given string to PZX output file.
 */
void pzx_browse( const void * const string, const uint length )
{
    pzx_flush() ;
    pzx_write_block( PZX_BROWSE, string, length ) ;
}

/**
 * Append PZX browse block using given string to PZX output file.
 */
void pzx_browse( const char * const string )
{
    hope( string ) ;
    pzx_browse( string, std::strlen( string ) ) ;
}

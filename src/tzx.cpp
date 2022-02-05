// $Id: tzx.cpp 1199 2009-05-12 08:32:36Z patrik $

/**
 * @file Rendering of TZX files.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#include "tzx.h"
#include "tap.h"
#include "csw.h"
#include "pzx.h"
#include "endian.h"

/**
 * Macros for fetching little endian data from current block.
 */
//@{
#define GET1(o)     (block[o])
#define GET2(o)     (block[o]+(block[(o)+1]<<8))
#define GET3(o)     (block[o]+(block[(o)+1]<<8)+(block[(o)+2]<<16))
#define GET4(o)     (block[o]+(block[(o)+1]<<8)+(block[(o)+2]<<16)+(block[(o)+3]<<24))
//@}

/**
 * Get size of the mandatory part of given block.
 */
uint tzx_get_header_size( const byte * const block )
{
    switch ( *block ) {
        case TZX_NORMAL_BLOCK:      return 1+0x04 ;
        case TZX_TURBO_BLOCK:       return 1+0x12 ;
        case TZX_PURE_TONE:         return 1+0x04 ;
        case TZX_PULSE_SEQUENCE:    return 1+0x01 ;
        case TZX_DATA_BLOCK:        return 1+0x0A ;
        case TZX_SAMPLES:           return 1+0x08 ;
        case TZX_PAUSE:             return 1+0x02 ;
        case TZX_GROUP_BEGIN:       return 1+0x01 ;
        case TZX_GROUP_END:         return 1+0x00 ;
        case TZX_JUMP:              return 1+0x02 ;
        case TZX_LOOP_BEGIN:        return 1+0x02 ;
        case TZX_LOOP_END:          return 1+0x00 ;
        case TZX_CALL_SEQUENCE:     return 1+0x02 ;
        case TZX_RETURN:            return 1+0x00 ;
        case TZX_SELECT_BLOCK:      return 1+0x02 ;
        case TZX_TEXT_INFO:         return 1+0x01 ;
        case TZX_MESSAGE:           return 1+0x02 ;
        case TZX_ARCHIVE_INFO:      return 1+0x02 ;
        case TZX_HARDWARE_INFO:     return 1+0x01 ;
        case TZX_CUSTOM_INFO:       return 1+0x14 ;
        case TZX_GLUE:              return 1+0x09 ;
        default:                    return 1+0x04 ;
    }
}

/**
 * Get size of the variable data part of given block.
 */
uint tzx_get_data_size( const byte * block )
{
    switch ( *block++ ) {
        case TZX_NORMAL_BLOCK:      return GET2(0x02) ;
        case TZX_TURBO_BLOCK:       return GET3(0x0F) ;
        case TZX_PURE_TONE:         return 0 ;
        case TZX_PULSE_SEQUENCE:    return GET1(0x00)*2 ;
        case TZX_DATA_BLOCK:        return GET3(0x07) ;
        case TZX_SAMPLES:           return GET3(0x05) ;
        case TZX_PAUSE:             return 0 ;
        case TZX_GROUP_BEGIN:       return GET1(0x00) ;
        case TZX_GROUP_END:         return 0 ;
        case TZX_JUMP:              return 0 ;
        case TZX_LOOP_BEGIN:        return 0 ;
        case TZX_LOOP_END:          return 0 ;
        case TZX_CALL_SEQUENCE:     return GET2(0x00)*2 ;
        case TZX_RETURN:            return 0 ;
        case TZX_SELECT_BLOCK:      return GET2(0x00) ;
        case TZX_TEXT_INFO:         return GET1(0x00) ;
        case TZX_MESSAGE:           return GET1(0x01) ;
        case TZX_ARCHIVE_INFO:      return GET2(0x00) ;
        case TZX_HARDWARE_INFO:     return GET1(0x00)*3 ;
        case TZX_CUSTOM_INFO:       return GET4(0x10) ;
        case TZX_GLUE:              return 0 ;
        default:                    return GET4(0) ;
    }
}

/**
 * Get pointer to block following after given block, or NULL in case of failure.
 */
const byte * tzx_get_next_block( const byte * const block, const byte * const tape_end )
{
    hope( tape_end ) ;
    hope( block ) ;

    // No next block in case we get beyond the end.

    if ( block >= tape_end ) {
        return NULL ;
    }

    // Otherwise get the header size and make sure it doesn't exceed the data available.

    const uint header_size = tzx_get_header_size( block ) ;
    hope( header_size > 0 ) ;

    if ( header_size > uint( tape_end - block ) ) {
        warn( "TZX block header size exceeds file size" ) ;
        return NULL ;
    }

    // Only then get the data size and make sure it doesn't exceed the data available either.

    const uint data_size = tzx_get_data_size( block ) ;

    if ( data_size > uint( tape_end - block - header_size ) ) {
        warn( "TZX block data size exceeds file size" ) ;
        return NULL ;
    }

    // Finally compute the position of the next block.

    return ( block + ( header_size + data_size ) ) ;
}

/**
 * Output pulse of given duration and level to the output stream and flip the level afterwards.
 */
void tzx_render_pulse( bool & level, const uint duration )
{
    // Send the pulse down the PZX stream.

    pzx_out( duration, level ) ;

    // Flip the level for the next pulse to come.

    level = ! level ;
}

/**
 * Output given amount of pulses of given duration to the output stream.
 */
void tzx_render_pulses( bool & level, const uint count, const uint duration )
{
    // Just output the pulses one by one.

    for ( uint i = 0 ; i < count ; i++ ) {
        tzx_render_pulse( level, duration ) ;
    }
}

/**
 * Output pilot tone using given arguments to the output stream.
 */
void tzx_render_pilot(
    bool & level,
    const uint leader_count,
    const uint leader_cycles,
    const uint sync_1_cycles,
    const uint sync_2_cycles
)
{
    tzx_render_pulses( level, leader_count, leader_cycles ) ;
    tzx_render_pulse( level, sync_1_cycles ) ;
    tzx_render_pulse( level, sync_2_cycles ) ;
}

/**
 * Output data block using given arguments to the output stream.
 */
void tzx_render_data(
    bool & level,
    const bool initial_level,
    const bool final_level_0,
    const bool final_level_1,
    const byte * const data,
    const uint bit_count,
    const uint pulse_count_0,
    const uint pulse_count_1,
    const word * const pulse_sequence_0,
    const word * const pulse_sequence_1,
    const uint tail_cycles,
    const uint pause_length
)
{
    // If there are some bits at all, output the data block.

    if ( bit_count > 0 ) {

        // Output the block.
        //
        // Note that we terminate the block with tail pulse in case the
        // pause is to follow. We do this always as we want the block to be
        // properly terminated regardless of the following blocks.

        pzx_data( data, bit_count, initial_level, pulse_count_0, pulse_count_1, pulse_sequence_0, pulse_sequence_1, pause_length > 0 ? tail_cycles : 0 ) ;

        // Adjust the current output level according to the last bit output.

        const uint bit_index = ( bit_count - 1 ) ;
        const uint bit_mask = ( 0x80 >> ( bit_index & 7 ) ) ;
        const uint last_byte = data[ bit_index / 8 ] ;
        level = ( ( ( last_byte & bit_mask ) != 0 ) ? final_level_1 : final_level_0 ) ;
    }

    // Now if there was some pause specified, output it as well.
    //
    // However don't output the pause if we have already used the tail pulse for that
    // and the pause was short enough. The output level is low after the pulse in either case, though.

    if ( pause_length > 0 ) {
        level = false ;
        if ( pause_length > 1 || tail_cycles == 0 || bit_count == 0 ) {
            pzx_pause( pause_length * MILLISECOND_CYCLES, level ) ;
        }
    }
}

/**
 * Output data block using given arguments to the output stream.
 */
void tzx_render_data(
    bool & level,
    const bool initial_level,
    const bool final_level_0,
    const bool final_level_1,
    const byte * const data,
    const uint data_size,
    const uint bits_in_last_byte,
    const uint bit_0_cycles_1,
    const uint bit_0_cycles_2,
    const uint bit_1_cycles_1,
    const uint bit_1_cycles_2,
    const uint tail_cycles,
    const uint pause_length
)
{
    // Compute the bit count, taking the amount of bits used in the last byte into account.

    uint bit_count = 8 * data_size ;

    if ( bits_in_last_byte <= 8 && bit_count >= 8 ) {
        bit_count -= 8 ;
        bit_count += bits_in_last_byte ;
    }

    // Prepare the pulse sequences for both bit 0 and 1.

    word s0[ 2 ] ;
    word s1[ 2 ] ;

    s0[ 0 ] = word( bit_0_cycles_1 ) ;
    s0[ 1 ] = word( bit_0_cycles_2 ) ;

    s1[ 0 ] = word( bit_1_cycles_1 ) ;
    s1[ 1 ] = word( bit_1_cycles_2 ) ;

    // Now output the block.

    tzx_render_data( level, initial_level, final_level_0, final_level_1, data, bit_count, 2, 2, s0, s1, tail_cycles, pause_length ) ;
}

/**
 * Output data block using given arguments to the output stream.
 */
void tzx_render_data(
    bool & level,
    const byte * const data,
    const uint data_size,
    const uint bits_in_last_byte,
    const uint bit_0_cycles,
    const uint bit_1_cycles,
    const uint tail_cycles,
    const uint pause_length
)
{
    tzx_render_data(
        level,
        level,
        level,
        level,
        data,
        data_size,
        bits_in_last_byte,
        bit_0_cycles,
        bit_0_cycles,
        bit_1_cycles,
        bit_1_cycles,
        tail_cycles,
        pause_length
    ) ;
}


/**
 * Output pause of given duration (in ms) to the output stream.
 */
void tzx_render_pause( bool & level, const uint duration )
{
    // Zero pause means no level change at all.

    if ( duration == 0 ) {
        return ;
    }

    // In case the level is high, leave it so for 1ms before bringing it low.

    if ( level ) {
        tzx_render_pulse( level, MILLISECOND_CYCLES ) ;
    }

    // Now output the low pulse pause of given duration.

    pzx_pause( duration * MILLISECOND_CYCLES, level ) ;
}

/**
 * Output pulses from given buffer, packing them to DATA block if possible.
 */
void tzx_render_gdb_pulses( const bool initial_level, Buffer & buffer, const uint sequence_limit, const uint sequence_order, const uint tail_cycles )
{
    const word * const pulses = buffer.get_typed_data< word >() ;
    const uint pulse_count = buffer.get_data_size() / 2 ;

    if ( ! pzx_pack( pulses, pulse_count, initial_level, sequence_limit, sequence_order, tail_cycles ) ) {
        pzx_pulses( pulses, pulse_count, initial_level, tail_cycles ) ;
    }

    buffer.clear() ;
}

/**
 * Output GDB symbol pulse sequence to given buffer.
 */
void tzx_render_gdb_symbol( bool & level, Buffer & buffer, const byte * sequence, const uint pulse_limit )
{
    // Adjust the output level.

    switch ( *sequence++ ) {
        case 0: {
            break ;
        }
        case 1: {
            buffer.write< word >( 0 ) ;
            level = ! level ;
            break ;
        }
        case 2: {
            if ( level ) {
                buffer.write< word >( 0 ) ;
            }
            level = false ;
            break ;
        }
        case 3: {
            if ( ! level ) {
                buffer.write< word >( 0 ) ;
            }
            level = true ;
            break ;
        }

        default: {
            warn( "invalid GDB pulse sequence level bits 0x%02x", sequence[ -1 ] ) ;
        }
    }

    // Now output the pulses.

    for ( uint i = 0 ; i < pulse_limit ; i++ ) {
        word duration = *sequence++ ;
        duration += *sequence++ << 8 ;
        if ( duration == 0 ) {
            break ;
        }
        buffer.write< word >( duration ) ;
        level = ! level ;
    }
}

/**
 * Decode GDB pilot pulses and send them to the output stream.
 */
void tzx_render_gdb_pilot(
    bool & level,
    Buffer & buffer,
    const byte * data,
    uint count,
    const byte * const table,
    const uint symbol_count,
    const uint symbol_pulses
)
{
    const bool initial_level = level ;

    // Output all pilot symbols.

    while ( count-- > 0 ) {

        // Fetch pilot symbol and verify it.

        const uint symbol = *data++ ;

        if ( symbol >= symbol_count ) {
            warn ( "pilot symbol %u is out of range <0,%u>", symbol, symbol_count - 1 ) ;
            continue ;
        }

        // Get the corresponding pulse sequence.

        const byte * const sequence = ( table + ( symbol * ( 2 * symbol_pulses + 1 ) ) ) ;

        // Get the repeat count.

        uint repeat_count = *data++ ;
        repeat_count += *data++ << 8 ;

        // Output the symbol as many times as needed.

        while ( repeat_count-- > 0 ) {
            tzx_render_gdb_symbol( level, buffer, sequence, symbol_pulses ) ;
        }
    }

    // Now simply send all the pulses to the output stream, without any
    // extra processing.

    tzx_render_gdb_pulses( initial_level, buffer, 0, 0, 0 ) ;
}

/**
 * Decode GDB data pulses and send them to the output stream.
 */
void tzx_render_gdb_data(
    bool & level,
    Buffer & buffer,
    const byte * data,
    uint count,
    const uint bit_count,
    const byte * const table,
    const uint symbol_count,
    const uint symbol_pulses,
    const uint pause_length
)
{
    const bool initial_level = level ;

    // Remember how to order the sequences depending on the first bit. Note that we use
    // this even in case of weird symbol counts, as the first bit will usually match
    // that of the intended sequence for given bit.

    const uint first_byte = ( count > 0 ? data[ 0 ] : 0 ) ;
    const uint first_bit = ( first_byte >> 7 ) ;
    const uint sequence_order = ( first_bit & 1 ) ;

    // Output all data symbols.

    uint mask = 0x80 ;

    while ( count-- > 0 ) {

        // Fetch data symbol and verify it.

        uint symbol = 0 ;
        for ( uint i = 0 ; i < bit_count ; i++ ) {
            symbol <<= 1 ;
            if ( ( *data & mask ) != 0 ) {
                symbol |= 1 ;
            }
            mask >>= 1 ;
            if ( mask == 0 ) {
                mask = 0x80 ;
                data++ ;
            }
        }

        if ( symbol >= symbol_count ) {
            warn ( "data symbol %u is out of range <0,%u>", symbol, symbol_count - 1 ) ;
            continue ;
        }

        // Get the corresponding pulse sequence.

        const byte * const sequence = ( table + ( symbol * ( 2 * symbol_pulses + 1 ) ) ) ;

        // Output the symbol.

        tzx_render_gdb_symbol( level, buffer, sequence, symbol_pulses ) ;
    }

    // Now try to pack the pulses to DATA block, and only if it fails,
    // output them as they are. Hint the packer about the maximum pulse
    // sequence allowed, including the possible extra zero pulse which was
    // perhaps added due to the forced level adjustments.
    //
    // Also try to use the tail pulse when possible, as it is preferred
    // form of finishing the final pulse.

    const uint tail_cycles = ( ( pause_length > 0 ) ? MILLISECOND_CYCLES : 0 ) ;

    tzx_render_gdb_pulses( initial_level, buffer, symbol_pulses + 1, sequence_order, tail_cycles ) ;

    // Now if there was some pause specified, output it as well.

    if ( pause_length > 0 ) {
        level = false ;
        pzx_pause( pause_length * MILLISECOND_CYCLES, level ) ;
    }
}

/**
 * Send the GDB block to the output stream.
 */
void tzx_render_gdb( bool & level, const byte * const block, const uint block_size )
{
    if ( block_size < 0x12 ) {
        warn( "TZX GDB block is too small" ) ;
        return ;
    }

    const byte * const block_end = ( block + 4 + block_size ) ;

    // Precompute the needed values.

    const uint pause_length = GET2(0x04) ;

    const uint pilot_symbols = GET4(0x06) ;
    const uint pilot_symbol_pulses = GET1(0x0A) ;
    const uint pilot_symbol_count = GET1(0x0B) ? GET1(0x0B) : 256 ;

    const uint data_symbols = GET4(0x0C) ;
    const uint data_symbol_pulses = GET1(0x10) ;
    const uint data_symbol_count = GET1(0x11) ? GET1(0x11) : 256 ;

    uint data_symbol_bits = 1 ;
    while( data_symbol_count > ( 1u << data_symbol_bits ) ) {
        data_symbol_bits++ ;
    }

    // Compute sizes and positions of the tables and streams.

    const uint pilot_table_size = ( pilot_symbols ? ( pilot_symbol_count * ( pilot_symbol_pulses * 2 + 1 ) ) : 0 ) ;
    const uint pilot_stream_size = ( pilot_symbols * 3 ) ;

    const uint data_table_size = ( data_symbols ? ( data_symbol_count * ( data_symbol_pulses * 2 + 1 ) ) : 0 ) ;
    const uint data_stream_size = ( ( ( data_symbols * data_symbol_bits ) + 7 ) / 8 ) ;

    const byte * const pilot_table = block + 0x12 ;
    const byte * const pilot_stream = pilot_table + pilot_table_size ;

    const byte * const data_table = pilot_stream + pilot_stream_size ;
    const byte * const data_stream = data_table + data_table_size ;

    const byte * const end = data_stream + data_stream_size ;

    // Verify the block size.

    if (
        block < pilot_table &&
        pilot_table <= pilot_stream &&
        pilot_stream <= data_table &&
        data_table <= data_stream &&
        data_stream <= end &&
        end <= block_end

    ) {
        if ( end != block_end ) {
            warn( "TZX GDB block contains unused data" ) ;
        }
    }
    else {
        warn( "TZX GDB block has invalid size" ) ;
        return ;
    }

    // Now render the pilot and the data, if either is present.

    Buffer buffer ;

    tzx_render_gdb_pilot( level, buffer, pilot_stream, pilot_symbols, pilot_table, pilot_symbol_count, pilot_symbol_pulses ) ;
    tzx_render_gdb_data( level, buffer, data_stream, data_symbols, data_symbol_bits, data_table, data_symbol_count, data_symbol_pulses, pause_length ) ;
}

/**
 * Send the CSW block to the output stream.
 */
void tzx_render_csw( bool & level, const byte * const block, const uint block_size )
{
    if ( block_size < 0x0E ) {
        warn( "TZX CSW block is too small" ) ;
        return ;
    }

    // Fetch the values.

    const uint pause_length = GET2(0x04) ;
    const uint sample_rate = GET3(0x06) ;
    const uint compression = GET1(0x09) ;
    const uint expected_pulse_count = GET4(0x0A) ;

    const byte * const data = block + 0xE ;
    const byte * const block_end = ( block + 4 + block_size ) ;

    if ( sample_rate == 0 ) {
        warn( "TZX CSW sample rate %u is invalid", sample_rate ) ;
        return ;
    }

    // Render the pulses.

    const uint pulse_count = csw_render_block( level, compression, sample_rate, data, block_end - data ) ;

    // Check the pulse count.

    if ( pulse_count != expected_pulse_count ) {
        warn( "TZX CSW block actual pulse count %u differs from expected pulse count %u", pulse_count, expected_pulse_count ) ;
    }

    // Adjust the level to remain the same as of the last pulse, not the opposite.

    if ( pulse_count > 0 ) {
        level = ! level ;
    }

    // Output the optional pause.

    tzx_render_pause( level, pause_length ) ;
}

/**
 * Get name of given info type.
 */
const char * tzx_get_info_name( const uint type )
{
    switch ( type ) {
        case 0x00: return "Title" ;
        case 0x01: return "Publisher" ;
        case 0x02: return "Author" ;
        case 0x03: return "Year" ;
        case 0x04: return "Language" ;
        case 0x05: return "Type" ;
        case 0x06: return "Price" ;
        case 0x07: return "Protection" ;
        case 0x08: return "Origin" ;
        case 0xFF: return "Comment" ;
        default:   return "Info" ;
    }
}

/**
 * Convert info from archive info block and pass it to the output stream.
 */
void tzx_convert_info( const byte * const info, const uint info_size, const bool title_only )
{
    hope( info ) ;

    // Fetch number of info strings.

    const byte * p = info ;
    const uint count = *p++ ;

    // Iterate over all strings.

    for ( uint i = 0 ; i < count ; i++ ) {

        // Make sure we don't run away from the info block.

        if ( p + 2 > info + info_size ) {
            break ;
        }

        // Fetch the info type and string length.

        const uint type = *p++ ;
        const uint length = *p++ ;
        const byte * const string = p ;

        // Move to next string.

        p += length ;

        if ( p > info + info_size ) {
            break ;
        }

        // Title is converted only when we are told so, otherwise it is ignored.

        if ( title_only ) {
            if ( type == 0x00 ) {
                pzx_info( string, length ) ;
                return ;
            }
            continue ;
        }
        else if ( type == 0x00 ) {
            continue ;
        }

        // Anything else is converted verbatim.
        //
        // Note that the output should be in UTF-8, but we don't know what
        // code page was used in TZX anyway, so we just let the user to fix
        // it himself if he cares enough.

        pzx_info( tzx_get_info_name( type ) ) ;
        pzx_info( string, length ) ;
    }

    // If we haven't found any title, just make up some if required.

    if ( title_only ) {
        pzx_info( "Some tape" ) ;
    }
}

/**
 * Set block index according to given relative offset.
 */
bool tzx_set_block_index( uint & block_index, const uint next_index, const sint offset, const uint block_count )
{
    hope( next_index > 0 ) ;

    // Make sure the offset doesn't jump further than allowed.

    block_index = next_index - 1 ;

    const uint limit = ( offset < 0 ? block_index : block_count - next_index ) ;
    const uint distance = uint( offset < 0 ? -offset : offset ) ;

    // In case it does, report error and proceed at next block.

    if ( distance > limit ) {
        block_index = next_index ;
        return false ;
    }

    // Otherwise jump to given block and report success.

    block_index += offset ;
    return true ;
}

// Forward declaration.

void tzx_process_blocks(
    bool & level,
    uint & block_index,
    const byte * const * const blocks,
    const uint block_count,
    const uint end_type,
    const uint nesting_level
) ;

/**
 * Process given TZX block.
 */
bool tzx_process_block(
    bool & level,
    uint & block_index,
    const byte * const * const blocks,
    const uint block_count,
    const uint end_type,
    const uint nesting_level,
    uint & jump_count
)
{
    hope( blocks ) ;
    hope( block_index < block_count ) ;

    // Fetch the block start.

    const byte * block = blocks[ block_index++ ] ;
    hope( block ) ;

    // Fetch the size of the block data.

    const uint data_size = tzx_get_data_size( block ) ;

    // Now process the block according to its type ID.

    switch ( *block++ ) {
        case TZX_NORMAL_BLOCK:
        {
            const uint leader_count = ( block[4] < 128 ? LONG_LEADER_COUNT : SHORT_LEADER_COUNT ) ;
            tzx_render_pilot( level, leader_count, LEADER_CYCLES, SYNC_1_CYCLES, SYNC_2_CYCLES ) ;
            tzx_render_data( level, block + 0x04, data_size, 8, BIT_0_CYCLES, BIT_1_CYCLES, TAIL_CYCLES, GET2(0x00) ) ;
            break ;
        }
        case TZX_TURBO_BLOCK:
        {
            tzx_render_pilot( level, GET2(0x0A), GET2(0x00), GET2(0x02), GET2(0x04) ) ;
            tzx_render_data( level, block + 0x12, data_size, GET1(0x0C), GET2(0x06), GET2(0x08), TAIL_CYCLES, GET2(0x0D) ) ;
            break ;
        }
        case TZX_PURE_TONE:
        {
            tzx_render_pulses( level, GET2(0x02), GET2(0x00) ) ;
            break ;
        }
        case TZX_PULSE_SEQUENCE:
        {
            uint count = *block++ ;
            while ( count-- > 0 ) {
                uint duration = *block++ ;
                duration += *block++ << 8 ;
                tzx_render_pulse( level, duration ) ;
            }
            break ;
        }
        case TZX_DATA_BLOCK:
        {
            tzx_render_data( level, block + 0x0A, data_size, GET1(0x04), GET2(0x00), GET2(0x02), TAIL_CYCLES, GET2(0x05) ) ;
            break ;
        }
        case TZX_SAMPLES:
        {
            const uint duration = GET2(0x00) ;
            tzx_render_data( level, false, false, true, block + 0x08, data_size, GET1(0x04), duration, 0, 0, duration, MILLISECOND_CYCLES, GET2(0x2) ) ;
            break ;
        }
        case TZX_CSW:
        {
            tzx_render_csw( level, block, data_size ) ;
            break ;
        }
        case TZX_GDB:
        {
            tzx_render_gdb( level, block, data_size ) ;
            break ;
        }
        case TZX_SET_LEVEL:
        {
            level = ( GET1(0x04) != 0 ) ;
            break ;
        }
        case TZX_PAUSE:
        {
            uint duration = GET2(0x00) ;
            if ( duration > 0 ) {
                tzx_render_pause( level, duration ) ;
            }
            else {
                pzx_stop( 0 ) ;
            }
            break ;
        }
        case TZX_STOP_IF_48K:
        {
            pzx_stop( 1 ) ;
            break ;
        }
        case TZX_GROUP_BEGIN:
        {
            pzx_browse( block + 1, data_size ) ;
            break ;
        }
        case TZX_GROUP_END:
        {
            break ;
        }
        case TZX_JUMP:
        {
            jump_count++ ;
            tzx_set_block_index( block_index, block_index, (s16) GET2(0x00), block_count ) ;
            break ;
        }
        case TZX_LOOP_BEGIN:
        {
            const uint count = GET2(0x00) ;
            const uint next_index = block_index ;
            for ( uint i = 0 ; i < count ; i++ ) {
                block_index = next_index ;
                tzx_process_blocks( level, block_index, blocks, block_count, TZX_LOOP_END, nesting_level ) ;
            }
            break ;
        }
        case TZX_LOOP_END:
        {
            if ( end_type == TZX_LOOP_END ) {
                return false ;
            }
            warn( "unexpected loop end block encountered" ) ;
            break ;
        }
        case TZX_CALL_SEQUENCE:
        {
            const uint count = GET2(0x00) ;
            const uint next_index = block_index ;
            for ( uint i = 0 ; i < count ; i++ ) {
                if ( ! tzx_set_block_index( block_index, next_index, (s16) GET2(0x02+2*i), block_count ) ) {
                    break ;
                }
                tzx_process_blocks( level, block_index, blocks, block_count, TZX_RETURN, nesting_level ) ;
            }
            block_index = next_index ;
            break ;
        }
        case TZX_RETURN:
        {
            if ( end_type == TZX_RETURN ) {
                return false ;
            }
            warn( "unexpected return block encountered" ) ;
            break ;
        }
        case TZX_SELECT_BLOCK:
        {
            warn( "select block was ignored" ) ;
            break ;
        }
        case TZX_TEXT_INFO:
        {
            pzx_browse( block + 1, data_size ) ;
            break ;
        }
        case TZX_MESSAGE:
        {
            warn( "message block was ignored" ) ;
            break ;
        }
        case TZX_ARCHIVE_INFO:
        {
            tzx_convert_info( block + 2, data_size, true ) ;
            tzx_convert_info( block + 2, data_size, false ) ;
            break ;
        }
        case TZX_HARDWARE_INFO:
        {
            warn( "hardware info block was ignored" ) ;
            break ;
        }
        case TZX_CUSTOM_INFO:
        {
            warn( "custom info block was ignored" ) ;
            break ;
        }
        case TZX_GLUE:
        {
            const uint major = GET1(0x07) ;
            const uint minor = GET1(0x08) ;
            if ( major != TZX_MAJOR ) {
                warn( "unsupported TZX major version %u.%u encountered - stopping", major, minor ) ;
                return false ;
            }
            if ( minor > TZX_MINOR ) {
                warn( "unsupported TZX minor revision %u.%u encountered - proceeding", major, minor ) ;
            }
            break ;
        }
        default: {
            warn( "unrecognized TZX block 0x%02x was ignored", block[-1] ) ;
            break ;
        }
    }

    return true ;
}

/**
 * Process given sequence of TZX blocks, stopping at block of given type.
 */
void tzx_process_blocks(
    bool & level,
    uint & block_index,
    const byte * const * const blocks,
    const uint block_count,
    const uint end_type,
    const uint nesting_level
)
{
    if ( nesting_level > 10 ) {
        warn( "too deep nesting detected - returning" ) ;
        return ;
    }

    // Simply process block by block, stopping when end block is encountered.

    uint jump_count = 0 ;

    while ( block_index < block_count ) {
        if ( ! tzx_process_block( level, block_index, blocks, block_count, end_type, nesting_level + 1, jump_count ) ) {
            break ;
        }
        if ( jump_count > block_count ) {
            warn( "too many jumps detected - stopping" ) ;
            break ;
        }
    }
}

/**
 * Render given TZX tape file to the PZX output stream.
 */
void tzx_render( const byte * const tape_start, const byte * const tape_end )
{
    hope( tape_start ) ;
    hope( tape_end ) ;
    hope( tape_start <= tape_end ) ;

    // Create table of block starts.

    Buffer block_buffer ;
    uint block_count = 0 ;

    const byte * block = tape_start ;

    while ( block < tape_end ) {

        block_buffer.write( block ) ;

        block = tzx_get_next_block( block, tape_end ) ;

        if ( block == NULL ) {
            break ;
        }

        block_count++ ;
    }

    const byte * const * const blocks = block_buffer.get_typed_data< const byte * >() ;

    // Now process process each block in turn.

    bool level = false ;
    uint block_index = 0 ;
    tzx_process_blocks( level, block_index, blocks, block_count, 0, 0 ) ;
}

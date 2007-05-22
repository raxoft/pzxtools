// $Id$

// Rendering of TZX files.

#include "tzx.h"
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

    if ( header_size > static_cast< uint >( tape_end - block ) ) {
        warn( "TZX block header size exceeds file size" ) ;
        return NULL ;
    }

    // Only then get the data size and make sure it doesn't exceed the data available either.

    const uint data_size = tzx_get_data_size( block ) ;

    if ( data_size > static_cast< uint >( tape_end - block - header_size ) ) {
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
    hope( duration < 0x80000000 ) ;

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

    // If there are some bits at all, output the data block.

    if ( bit_count > 0 ) {

        // Prepare the pulse sequences for both bit 0 and 1.

        u16 s0[ 2 ] ;
        u16 s1[ 2 ] ;

        s0[ 0 ] = little_endian< u16 >( bit_0_cycles_1 ) ;
        s0[ 1 ] = little_endian< u16 >( bit_0_cycles_2 ) ;

        s1[ 0 ] = little_endian< u16 >( bit_1_cycles_1 ) ;
        s1[ 1 ] = little_endian< u16 >( bit_1_cycles_2 ) ;

        // Output the block.
        //
        // Note that we terminate the block with tail pulse in case the
        // pause is to follow. We do this always as we want the block to be
        // properly terminated regardless of the following blocks.

        pzx_data( data, bit_count, level, 2, 2, s0, s1, pause_length > 0 ? tail_cycles : 0 ) ;
    }

    // Now if there was some pause specified, output it as well.

    if ( pause_length > 0 ) {
        level = false ;
        pzx_pause( pause_length * MILLISECOND_CYCLES, level ) ;
    }
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
 * Output pause of given duration to the output stream.
 */
void tzx_render_pause( bool & level, const uint duration )
{
    hope( duration > 0 ) ;

    // In case the level is high, leave it so for 1ms before bringing it low.

    if ( level ) {
        tzx_render_pulse( level, MILLISECOND_CYCLES ) ;
    }

    // Now output the low pulse pause of given duration.

    pzx_pause( duration * MILLISECOND_CYCLES, level ) ;
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
    const uint distance = static_cast< uint >( offset < 0 ? -offset : offset ) ;

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
    const uint end_type
) ;

/**
 * Process given TZX block.
 */
bool tzx_process_block(
    bool & level,
    uint & block_index,
    const byte * const * const blocks,
    const uint block_count,
    const uint end_type
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
            level = false ;
            tzx_render_data( level, block + 0x08, data_size, GET1(0x04), duration, 0, 0, duration, MILLISECOND_CYCLES, GET2(0x2) ) ;
            // FIXME: level is now invalid unless there was a pause, as it doesn't reflect the last bit output.
            break ;
        }
        case TZX_CSW:
        {
            warn( "CSW block not supported yet" ) ;
            break ;
        }
        case TZX_GDB:
        {
            warn( "GDB block not supported yet" ) ;
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
            tzx_set_block_index( block_index, block_index, (s16) GET2(0), block_count ) ;
            break ;
        }
        case TZX_LOOP_BEGIN:
        {
            const uint count = GET2(0x00) ;
            const uint next_index = block_index ;
            for ( uint i = 0 ; i < count ; i++ ) {
                block_index = next_index ;
                tzx_process_blocks( level, block_index, blocks, block_count, TZX_LOOP_END ) ;
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
                if ( ! tzx_set_block_index( block_index, next_index, (s16) GET2(2+2*i), block_count ) ) {
                    break ;
                }
                tzx_process_blocks( level, block_index, blocks, block_count, TZX_RETURN ) ;
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
            // FIXME: use this to feed the header.
            warn( "archive info block not supported yet" ) ;
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
    const uint end_type
)
{
    // Simply process block by block, stopping when end block is encountered.

    while ( block_index < block_count ) {
        if ( ! tzx_process_block( level, block_index, blocks, block_count, end_type ) ) {
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

    const uint block_limit = 4096 ;
    const byte * blocks[ block_limit ] ;
    uint block_count = 0 ;

    const byte * block = tape_start ;

    while ( block < tape_end ) {

        blocks[ block_count ] = block ;

        block = tzx_get_next_block( block, tape_end ) ;

        if ( block == NULL ) {
            break ;
        }

        if ( block_count == block_limit ) {
            warn( "too many TZX blocks encountered, the rest is ignored" ) ;
            break ;
        }

        block_count++ ;
    }

    // Now process process each block in turn.

    bool level = false ;
    uint block_index = 0 ;
    tzx_process_blocks( level, block_index, blocks, block_count, 0 ) ;
}

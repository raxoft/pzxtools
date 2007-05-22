// $Id$

// Rendering of TZX files.

#include "tzx.h"
#include "pzx.h"

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

uint tzx_get_data_size( const byte * const block )
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

const byte * tzx_get_next_block( const byte * const block, const byte * const tape_end )
{
    hope( tape_end ) ;
    hope( block ) ;
    
    if ( block >= tape_end ) {
        return NULL ;
    }

    const uint header_size = tzx_get_header_size( block ) ;
    hope( header_size > 0 ) ;

    if ( header_size > static_cast< uint >( tape_end - block ) ) {
        return NULL ;
    }
    
    const uint data_size = tzx_get_data_size( block ) ;
    
    if ( data_size > static_cast< uint >( tape_end - block - header_size ) ) {
        return NULL ;
    }
    
    return ( block + ( header_size + data_size ) ) ;
}

const byte * tzx_get_block( const byte * const tape_start, const byte * const tape_end, const uint index )
{
    hope( tape_start ) ;
    hope( tape_end ) ;
    
    const byte * block = tape_start ;
    
    for ( uint i = 0 ; i < index ; i++ ) {
        block = tzx_get_next_block( block, tape_end ) ;
        if ( block == NULL ) {
            break ;
        }
    }
    
    return block ;
}

uint tzx_get_block_index( const byte * const tape_start, const byte * const tape_end, const byte * const target_block )
{
    hope( tape_start ) ;
    hope( tape_end ) ;
    hope( target_block ) ;
    
    uint index = 0 ;

    const byte * block = tape_start ;
    
    while ( block < tape_end ) {
    
        block = tzx_get_next_block( block, tape_end ) ;
        
        if ( block == NULL ) {
            break ;
        }
        
        if ( block == target_block ) {
            break ;
        }
    
        index++ ;
    }
    
    return index ;
}

const byte * tzx_find_block( const byte * const tape_start, const byte * const tape_end, 

void tzx_render_pulse( bool & level, const uint duration )
{
    hope( duration < 0x80000000 ) ;
    
    pzx_out( duration, level ) ;
    level = ! level ;
}

void tzx_render_pulses( bool & level, const uint count, const uint duration )
{
    for ( uint i = 0 ; i < count ; i++ ) {
        tzx_render_pulse( level, duration ) ;
    }
}

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

void tzx_render_data(
    bool & level,
    const byte * const data,
    const uint data_size,
    const uint bits_in_last_byte,
    const uint bit_0_cycles_1,
    const uint bit_0_cycles_2,
    const uint bit_1_cycles_1,
    const uint bit_1_cycles_2,
    const uint pause_length
)
{
    uint bit_count = 8 * data_size ;
    
    if ( bits_in_last_byte <= 8 && bit_count >= 8 ) {
        bit_count -= 8 ;
        bit_count += bits_in_last_byte ;
    }
    
    if ( bit_count > 0 ) {

        u16 s0[ 2 ] ;
        u16 s1[ 2 ] ;
        
        s0[ 0 ] = little_endian< u16 >( bit_0_cycles_1 ) ;
        s0[ 1 ] = little_endian< u16 >( bit_0_cycles_2 ) ;

        s1[ 0 ] = little_endian< u16 >( bit_1_cycles_1 ) ;
        s1[ 1 ] = little_endian< u16 >( bit_1_cycles_2 ) ;
        
        const uint tail_cycles = ( pause_length > 0 ? MILLISECOND_CYCLES : 0 ) ;
        
        pzx_data( data, bit_count, level, 2, 2, s0, s1, tail_cycles ) ;
    }
        
    if ( pause_length > 0 ) {
        level = false ;
        pzx_pause( pause_length * MILLISECOND_CYCLES, level ) ;
    }
}

void tzx_render_data(
    bool & level,
    const byte * const data,
    const uint data_size,
    const uint bits_in_last_byte,
    const uint bit_0_cycles,
    const uint bit_1_cycles,
    const uint pause_length
)
{
    tzx_render_data( level, data, data_size, bits_in_last_byte, bit_0_cycles, bit_0_cycles, bit_1_cycles, bit_1_cycles, pause_length ) ;
}

const byte * tzx_process_block(
    bool & level,
    const byte * block,
    const uint end_type,
    const byte * const tape_start,
    const byte * const tape_end,
)
{
    hope( tape_start ) ;
    hope( tape_end ) ;
    hope( tape_start <= tape_end ) ;
    hope( block ) ;
    hope( block >= tape_start ) ;
    hope( block <= tape_end ) ;

    const byte * const next_block = tzx_get_next_block( block, tape_end ) ;

    if ( next_block == NULL ) {
        return NULL ;
    }

    switch ( *block++ ) {
        case TZX_NORMAL_BLOCK:
        {
            tzx_render_pilot( level, LEADER_COUNT, LEADER_CYCLES, SYNC_1_CYCLES, SYNC_2_CYCLES ) ;
            tzx_render_data( level, block + 0x04, data_size, 0, BIT_0_CYCLES, BIT_1_CYCLES, GET2(0x00) ) ;
            break ;
        }
        case TZX_TURBO_BLOCK:
        {
            tzx_render_pilot( level, GET2(0x0A), GET2(0x00), GET2(0x02), GET2(0x04) ) ;
            tzx_render_data( level, block + 0x12, data_size, GET1(0x0C), GET2(0x06), GET2(0x08), GET2(0x0D) ) ;
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
            tzx_render_data( level, block + 0x0A, data_size, GET1(0x04), GET2(0x00), GET2(0x02), GET2(0x05) ) ;
            break ;
        }
        case TZX_SAMPLES:
        {
            const uint duration = GET2(0x00) ;
            level = false ;
            tzx_render_data( level, block + 0x08, data_size, GET1(0x04), duration, 0, 0, duration, GET2(0x2) ) ;
            level = ! level ; // FIXME: last level, in case sample block is empty or there is just pause...
            break ;
        }
        case TZX_CSW:
            return NULL ;   // Not yet.
        case TZX_GDB:
            return NULL ;   // Not yet.
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
            // return tzx_find_block( tape_start, tape_end, block_index + (s16) GET2(0) ) ; // FIXME.
            break ;
        }
        case TZX_LOOP_BEGIN:
        {
            break ; // FIXME.
        }
        case TZX_LOOP_END:
        {
            if ( end_type != TZX_LOOP_END ) {
                return NULL ;
            }
            break ;
        }
        case TZX_CALL_SEQUENCE:
            break ; // FIXME
        case TZX_RETURN:
        {
            if ( end_type != TZX_RETURN ) {
                return NULL ;
            }
            break ;
        }
        case TZX_SELECT_BLOCK:
        {
            break ;
        }
        case TZX_TEXT_INFO:
        {
            break ;
        }
        case TZX_MESSAGE:
        {
            break ;
        }
        case TZX_ARCHIVE_INFO:
        {
            break ;
        }
        case TZX_HARDWARE_INFO:
        {
            break ;
        }
        case TZX_CUSTOM_INFO:
        {
            break ;
        }
        case TZX_GLUE:
        {
            const uint major = GET1(0x07) ;
            const uint minor = GET1(0x08) ;
            if ( major != TZX_MAJOR ) {
                // FIXME: print error in this case.
                return NULL ;
            }
            if ( minor > TZX_MINOR ) {
                // FIXME: Print warning in this case.
            }
            break ;
        }
    }
    
    return next_block ;
}

const byte * tzx_process_blocks(
    bool & level,
    const byte * const first_block,
    const uint end_type,
    const byte * const tape_start,
    const byte * const tape_end,
)
{
    const byte * block = first_block ;

    while ( block < tape_end ) {
    
        const uint block_type = *block ;
    
        block = tzx_process_block( level, block, end_type, tape_start, tape_end ) ;
        
        if ( block == NULL ) {
            break ;
        }
        
        if ( block_type == end_type ) {
            break ;
        }
    }
    
    return block ;
}

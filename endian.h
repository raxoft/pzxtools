// $Id$

#ifndef ENDIAN_H
#define ENDIAN_H

#ifndef TYPES_H
#include "types.h"
#endif

/**
 * Swap the endian order of given value.
 */
//@{

template< typename Type >
inline Type swapped_endian( const Type value ) ;

template<>
inline u8 swapped_endian( const u8 value )
{
    return value ;
}

template<>
inline s8 swapped_endian( const s8 value )
{
    return value ;
}

template<>
inline u16 swapped_endian( const u16 value )
{
    return static_cast< u16 >(
        ( ( value & 0x00FF ) << 8 ) |
        ( value >> 8 )
    ) ;
}

template<>
inline s16 swapped_endian( const s16 value )
{
    return static_cast< s16 >( swapped_endian( static_cast< u16 >( value ) ) ) ;
}

template<>
inline u32 swapped_endian( const u32 value )
{
    return static_cast< u32 >(
        ( ( value & 0x000000FF ) << 24 ) |
        ( ( value & 0x0000FF00 ) << 8 ) |
        ( ( value & 0x00FF0000 ) >> 8 ) |
        ( value >> 24 )
    ) ;
}

template<>
inline s32 swapped_endian( const s32 value )
{
    return static_cast< s32 >( swapped_endian( static_cast< u32 >( value ) ) ) ;
}

template<>
inline f32 swapped_endian( const f32 value )
{
    union {
        f32 as_f32 ;
        u32 as_u32 ;
    } t ;
    t.as_f32 = value ;
    t.as_u32 = swapped_endian( t.as_u32 ) ;
    return t.as_f32 ;
}

//@}

/**
 * Keep given value in native endian order.
 *
 * Used for documentation purposes, that the conversion was not ommited by mistake
 * and that the value should really remain in the native endian order.
 */
template< typename Type >
inline Type native_endian( const Type value )
{
    return value ;
}

#if ( __BYTE_ORDER == __LITTLE_ENDIAN )

/**
 * Convert given value to/from little endian order from/to native endian order.
 */
template< typename Type >
inline Type little_endian( const Type value )
{
    return native_endian( value ) ;
}

/**
 * Convert given value to/from big endian order from/to native endian order.
 */
template< typename Type >
inline Type big_endian( const Type value )
{
    return swapped_endian( value ) ;
}

#elif ( __BYTE_ORDER == __BIG_ENDIAN )

/**
 * Convert given value to/from little endian order from/to native endian order.
 */
template< typename Type >
inline Type little_endian( const Type value )
{
    return swapped_endian( value ) ;
}

/**
 * Convert given value to/from big endian order from/to native endian order.
 */
template< typename Type >
inline Type big_endian( const Type value )
{
    return native_endian( value ) ;
}

#else
#error "Unknown __BYTE_ORDER."
#endif

#endif // ENDIAN_H

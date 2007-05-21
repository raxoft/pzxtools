// $Id$

// Self-inflating buffer.

#ifndef BUFFER_H
#define BUFFER_H 1

#include <cstdlib>
#include <cstring>

#ifndef TYPES_H
#include "types.h"
#endif

/**
 * Trivial class for convenient storing of arbitrary data.
 */
class Buffer {

    byte * buffer ;
    uint buffer_size ;
    uint bytes_used ;

public:

    Buffer( const uint size = 65536 )
        : buffer( NULL )
        , buffer_size( 0 )
        , bytes_used( 0 )
    {
        reallocate( size ) ;
    }

    ~Buffer()
    {
        std::free( buffer ) ;
    }

private:

    void reallocate( const uint new_size )
    {
        hope( new_size > buffer_size ) ;

        buffer = std::realloc( buffer, new_size ) ;
        hope( buffer ) ;

        buffer_size = new_size ;
    }

public:

    inline void clear( void )
    {
        bytes_used = 0 ;
    }

public:

    inline void write( const void * const data, const uint size )
    {
        hope( data || size == 0 ) ;

        while ( size > buffer_size - bytes_used ) {
            reallocate( 2 * buffer_size ) ;
        }

        std::memcpy( buffer + bytes_used, data, size ) ;

        bytes_used += size ;
    }

    template< typename Type >
    inline void write_native( const Type value )
    {
        write( &value, sizeof( value ) ) ;
    }

    template< typename Type >
    inline void write_little( const Type value )
    {
        write_native( little_endian( value ) ) ;
    }

    template< typename Type >
    inline void write_big( const Type value )
    {
        write_native( big_endian( value ) ) ;
    }

public:

    inline byte * get_data( void ) const
    {
        return buffer ;
    }

    inline uint get_data_size( void ) const
    {
        return bytes_used ;
    }

    inline bool is_empty( void ) const
    {
        return ( bytes_used == 0 ) ;
    }

    inline bool is_not_empty( void ) const
    {
        return ( bytes_used > 0 ) ;
    }

} ;

#endif // BUFFER_H

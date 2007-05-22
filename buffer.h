// $Id$

// Self-inflating buffer.

#ifndef BUFFER_H
#define BUFFER_H 1

#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifndef DEBUG_H
#include "debug.h"
#endif

#ifndef ENDIAN_H
#include "endian.h"
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

        buffer = static_cast< byte * >( std::realloc( buffer, new_size ) ) ;
        hope( buffer ) ;

        buffer_size = new_size ;
    }

public:

    inline void clear( void )
    {
        bytes_used = 0 ;
    }

public:

    bool read( FILE * const file )
    {
        hope( file ) ;

        for ( ; ; ) {

            const uint bytes_free = buffer_size - bytes_used ;
            const uint bytes_read = std::fread( buffer + bytes_used, 1, bytes_free, file ) ;

            bytes_used += bytes_read ;

            if ( bytes_read != bytes_free ) {
                return ( ferror( file ) == 0 ) ;
            }

            reallocate( 2 * buffer_size ) ;
        }
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
    inline void write( const Type value )
    {
        write( &value, sizeof( value ) ) ;
    }

    template< typename Type >
    inline void write_little( const Type value )
    {
        write( little_endian( value ) ) ;
    }

    template< typename Type >
    inline void write_big( const Type value )
    {
        write( big_endian( value ) ) ;
    }

public:

    inline byte * get_data( void ) const
    {
        return buffer ;
    }

    inline byte * get_data_end( void ) const
    {
        return buffer + bytes_used ;
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

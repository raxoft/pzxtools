// $Id: tzx.h 302 2007-06-15 07:37:58Z patrik $

/**
 * @file TZX stuff.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#ifndef TZX_H
#define TZX_H 1

#ifndef TYPES_H
#include "types.h"
#endif

// TZX version we support.

const byte TZX_MAJOR = 1 ;
const byte TZX_MINOR = 20 ;

// TZX block IDs.

const byte TZX_NORMAL_BLOCK     = 0x10 ;
const byte TZX_TURBO_BLOCK      = 0x11 ;
const byte TZX_PURE_TONE        = 0x12 ;
const byte TZX_PULSE_SEQUENCE   = 0x13 ;
const byte TZX_DATA_BLOCK       = 0x14 ;
const byte TZX_SAMPLES          = 0x15 ;
const byte TZX_CSW              = 0x18 ;
const byte TZX_GDB              = 0x19 ;
const byte TZX_PAUSE            = 0x20 ;
const byte TZX_GROUP_BEGIN      = 0x21 ;
const byte TZX_GROUP_END        = 0x22 ;
const byte TZX_JUMP             = 0x23 ;
const byte TZX_LOOP_BEGIN       = 0x24 ;
const byte TZX_LOOP_END         = 0x25 ;
const byte TZX_CALL_SEQUENCE    = 0x26 ;
const byte TZX_RETURN           = 0x27 ;
const byte TZX_SELECT_BLOCK     = 0x28 ;
const byte TZX_STOP_IF_48K      = 0x2A ;
const byte TZX_SET_LEVEL        = 0x2B ;
const byte TZX_TEXT_INFO        = 0x30 ;
const byte TZX_MESSAGE          = 0x31 ;
const byte TZX_ARCHIVE_INFO     = 0x32 ;
const byte TZX_HARDWARE_INFO    = 0x33 ;
const byte TZX_CUSTOM_INFO      = 0x35 ;
const byte TZX_GLUE             = 0x5A ;

// Interface.

void tzx_render( const byte * const tape_start, const byte * const tape_end ) ;

#endif // TZX_H

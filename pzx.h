// $Id$

// PZX stuff.

#ifndef PZX_H
#define PZX_H 1

#ifndef TYPES_H
#include "types.h"
#endif

// PZX version we support.

const byte PZX_MAJOR = 0 ;
const byte PZX_MINOR = 3 ;

// PZX block tags.

const uint PZX_HEADER   = 'ZXTP' ;
const uint PZX_PULSES   = 'PULS' ;
const uint PZX_DATA     = 'DATA' ;
const uint PZX_PAUSE    = 'PAUS' ;
const uint PZX_STOP     = 'STOP' ;
const uint PZX_BROWSE   = 'BRWS' ;

#endif // PZX_H

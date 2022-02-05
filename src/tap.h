// $Id: tap.h 302 2007-06-15 07:37:58Z patrik $

/**
 * @file TAP constants.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#ifndef TAP_H
#define TAP_H 1

#ifndef TYPES_H
#include "types.h"
#endif

// Default loader constants.

const uint LEADER_CYCLES        = 2168 ;
const uint SHORT_LEADER_COUNT   = 3223 ;
const uint LONG_LEADER_COUNT    = 8063 ;
const uint SYNC_1_CYCLES        = 667 ;
const uint SYNC_2_CYCLES        = 735 ;
const uint BIT_0_CYCLES         = 855 ;
const uint BIT_1_CYCLES         = 1710 ;
const uint TAIL_CYCLES          = 945 ;
const uint MILLISECOND_CYCLES   = 3500 ;

#endif // TAP_H

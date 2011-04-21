#! /usr/bin/perl -w
#
# Expand repeated pulses, so each pulse is on a separate line.
#
# This is the same as if pzx2txt -e was used in the first place.
#
# Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

while(<>) {

    if ( my( $duration, $count ) = /^PULSE\d?\s+(\d+)\s+(\d+)/i ) {
        while( $count-- > 0 ) {
            print "PULSE $duration\n" ;
        }
        next ;
    }

    print ;
}

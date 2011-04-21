#! /usr/bin/perl -w
#
# Annotate pulses with their initial pulse level.
#
# This is the same as if pzx2txt -l was used in the first place.
#
# Copyright (C) 2011 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

my $level = 0 ;

while(<>) {

    if ( my( $duration, $count ) = /^PULSE\d?\s+(\d+)\s*(\d*)/i ) {

        $count = 1 if $count eq "" ;

        print "PULSE$level $duration", ( $count > 1 ? " $count" : "" ), "\n" ;

        $level ^= ( $count & 1 ) ;

        next ;
    }

    if ( /^(DATA|PACK)\s+(\d+)/i ) {
        $level = ( $2 != 0 ) ;
    }
    elsif ( /^(PULSES|DATA)/ ) {
        $level = 0 ;
    }

    print ;
}

#! /usr/bin/perl -wT
#
# Expand repeated pulses, so each pulse is on a separate line.
#
# This is the same as if pzx2txt -e was used in the first place.

while(<>) {

    if ( my( $duration, $count ) = /^PULSE\s+(\d+)\s+(\d+)/i ) {
        while( $count-- > 0 ) {
            print "PULSE $duration\n" ;
        }
        next ;
    }

    print ;
}

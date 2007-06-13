#! /usr/bin/perl -wT
#
# Convert PAUSE blocks to normal PULSE blocks.

while(<>) {
    if ( my( $duration, $level ) = /^PAUSE\s+(\d+)\s*(\d*)/ ) {
        print "PULSES\n" ;
        print "PULSE 0\n" if $level ;
        print "PULSE $duration\n" ;
        next ;
    }
    print ;
}

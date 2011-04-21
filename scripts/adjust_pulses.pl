#! /usr/bin/perl -w
#
# Adjust pulse pulse durations by given scale factor and bias.
#
# Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

my @scale = ( 1, 1 ) ;
my @bias = ( 0, 0 ) ;
my $level = 0 ;

while(<>) {

    if ( my( $orig_level, $duration, $count, $rest ) = /^PULSE(\d?)\s+(\d+)\s*(\d*)(.*)$/i ) {

        $count = 1 if $count eq "" ;

        die "please expand pulses with expand_pulses.pl or pzx2txt -e\n"
            if $count > 1 && ( $scale[0] != $scale[1] || $bias[0] != $bias[1] ) ;

        $duration = $duration * $scale[ $level ] + $bias[ $level ] ;

        if ( $duration < 0 ) {
            $duration = 0 ;
        }
        elsif ( $duration > 0xFFFFFFFF ) {
            $duration = 0xFFFFFFFF ;
        }
        else {
            $duration = int( $duration ) ;
        }

        print "PULSE$orig_level $duration", ( $count > 1 ? " $count" : "" ), "$rest\n" ;

        $level ^= ( $count & 1 ) ;

        next ;
    }

    if ( my( $a, $b ) = /^SCALE\s*(\d*\.?\d*)\s*(\d*\.?\d*)/i ) {
        $a = 1 if $a eq "" ;
        $b = $a if $b eq "" ;
        @scale = ( $a, $b ) ;
        next ;
    }

    if ( my( $a, $b ) = /^BIAS\s*(-?\d*)\s*(-?\d*)/i ) {
        $a = 0 if $a eq "" ;
        $b = $a if $b eq "" ;
        @bias = ( $a, $b ) ;
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

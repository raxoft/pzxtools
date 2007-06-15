#! /usr/bin/perl -wT
#
# Map pulse durations in given ranges to specific values.
#
# Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

my @filters ;

loop: while(<>) {

    if ( my( $duration, $rest ) = /^PULSE\s+(\d+)(.*)$/i ) {
        my @f = @filters ;
        while ( my( $result, $min, $max ) = splice( @f, 0, 3 ) ) {
            if ( $min <= $duration && $duration <= $max ) {
                print "PULSE $result$rest\n" ;
                next loop ;
            }
        }
        print ;
        next ;

    }

    if ( my( $duration, $a, $b ) = /^FILTER\s+(\d+)\s+(\d+)\s*(\d*)/i ) {
        if ( $b eq "" ) {
            push( @filters, $duration, $duration - $a, $duration + $a ) ;
        }
        else {
            push( @filters, $duration, $a, $b ) ;
        }
        next ;
    }

    if ( /^FILTER\s*$/i ) {
        @filters = () ;
        next ;
    }

    print ;
}

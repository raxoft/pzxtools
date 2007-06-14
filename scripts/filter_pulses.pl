#! /usr/bin/perl -wT
#
# Map pulse durations in given range to specific value.

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

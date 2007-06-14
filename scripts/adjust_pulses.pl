#! /usr/bin/perl -wT
#
# Adjust pulse pulse durations by given scale factor and bias.

my @scale = ( 1.0, 1.0 ) ;
my @bias = ( 0, 0 ) ;

my $level = 0 ;

loop: while(<>) {

    if ( my( $duration, $count ) = /^PULSE\s+(\d+)\s*(\d*)$/i ) {

        die "please expand pulses with expand_pulses.pl or pzx2txt -e\n"
            if $count ne "" ;

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

        print "PULSE $duration\n" ;

        $level = ! $level ;

        next ;
    }

    if ( /^PULSES/ ) {
        $level = 0 ;
    }

    print ;
}

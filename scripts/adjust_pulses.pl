#! /usr/bin/perl -wT
#
# Adjust pulse pulse durations by given scale factor and bias.

my @scale = ( 1, 1 ) ;
my @bias = ( 0, 0 ) ;

my $level = 0 ;

loop: while(<>) {

    if ( my( $duration, $count ) = /^PULSE\s+(\d+)\s*(\d*)$/i ) {

        die "please expand pulses with expand_pulses.pl or pzx2txt -e\n"
            if $count ne "" && $count > 1 && ( $scale[0] != $scale[1] || $bias[0] != $bias[1] ) ;

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

        print "PULSE $duration $count\n" ;

        $level = ! $level ;

        next ;
    }

    if ( my( $a, $b ) = /^SCALE\s*(\d*\.?\d*)\s*(\d*\.?\d*)/i ) {
        $a = 1 if $a eq "" ;
        $b = $a if $b eq "" ;
        @scale = ( $a, $b ) ;
        next ;
    }

    if ( my( $a, $b ) = /^BIAS\s*(\d*)\s*(\d*)/i ) {
        $a = 0 if $a eq "" ;
        $b = $a if $b eq "" ;
        @bias = ( $a, $b ) ;
        next ;
    }

    if ( /^PULSES/ ) {
        $level = 0 ;
    }
    elsif ( /^(PACK|DATA)\s+(\d+)/i ) {
        $level = ( $2 != 0 ) ;
    }

    print ;
}

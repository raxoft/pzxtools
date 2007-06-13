#! /usr/bin/perl -wT
#
# Convert short sequences of pulses to PAUSE blocks.

my $pulse_limit = shift or die "usage: $0 <pulse_limit>\n" ;

my $pause_duration = 0 ;

my @pulses = () ;

while(<>) {

    die "expand pulses with expand_pulses.pl or pzx2txt -e\n" if /^PULSE\s+\d+\s+\d+/i ;

    next if /^s*$/ && @pulses > 0 ;

    if ( /^PULSES/i ) {
        push( @pulses, $_ ) ;
        next ;
    }

    if ( my( $duration ) = /^PULSE\s+(\d+)/i ) {
        push( @pulses, $_ ) ;
        $pause_duration += $duration ;
        next ;
    }

    if ( $pause_duration > 0 ) {
        if ( @pulses >= $pulse_limit ) {
            print @pulses ;
        }
        else {
            print "PAUSE $pause_duration\n" ;
        }
        @pulses = () ;
        $pause_duration = 0 ;
    }

    print ;
}

if ( $pause_duration > 0 ) {
    print @pulses ;
}

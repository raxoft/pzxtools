#! /usr/bin/perl -wT
#
# Convert sequences of long pulses to PAUSE blocks.

my $pulse_limit = shift or die "usage: $0 <pulse_limit>\n" ;

my @pulses ;

my $pause_duration = 0 ;

my $level = 0 ;

while(<>) {

    die "expand pulses with expand_pulses.pl or pzx2txt -e\n" if /^PULSE\s+\d+\s+\d+/i ;

    $level = 0 if /^PULSES/i ;

    if ( my( $duration ) = /^PULSE\s+(\d+)/i ) {
        $level = ! $level ;
        if ( $duration >= $pulse_limit ) {
            $pause_duration += $duration ;
            next ;
        }
    }

    if ( $pause_duration > 0 ) {
        print "\n" ;
        print "PAUSE $pause_duration\n" ;
        print "\n" ;
        print "PULSES\n" ;
        print "PULSE 0\n" unless $level ;
        $pause_duration = 0 ;
    }

    print ;
}

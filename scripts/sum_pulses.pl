#! /usr/bin/perl -wT
#
# Compute total duration of pulses encountered.

my $sum = 0 ;

while(<>) {

    next unless ( my( $duration, $count ) = /^PULSE\s+(\d+)\s*(\d*)/i ) ;

    $count = 1 if $count eq "" ;

    $sum += $duration * $count ;
}

print "PULSE $sum\n" ;

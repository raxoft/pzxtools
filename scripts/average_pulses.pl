#! /usr/bin/perl -wT
#
# Compute average duration of pulses encountered.

my $sum = 0 ;
my $total = 0 ;

while(<>) {

    next unless ( my( $duration, $count ) = /^PULSE\s+(\d+)\s*(\d*)/i ) ;

    next if $duration == 0 ;

    $count = 1 if $count eq "" ;

    $sum += $duration * $count ;
    $total += $count ;
}

my $average = int( $sum / $total ) ;

print "PULSE $average $total\n" ;

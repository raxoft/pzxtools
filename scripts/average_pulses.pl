#! /usr/bin/perl -w
#
# Compute average duration of pulses encountered.
#
# Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

my $sum = 0 ;
my $total = 0 ;

while(<>) {

    next unless ( my( $duration, $count ) = /^PULSE\d?\s+(\d+)\s*(\d*)/i ) ;

    next if $duration == 0 ;

    $count = 1 if $count eq "" ;

    $sum += $duration * $count ;
    $total += $count ;
}

my $average = $total && int( $sum / $total ) ;

print "PULSE $average $total\n" ;

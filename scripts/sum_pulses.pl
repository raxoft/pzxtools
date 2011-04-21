#! /usr/bin/perl -w
#
# Compute total duration of pulses encountered.
#
# Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

my $sum = 0 ;

while(<>) {

    next unless ( my( $duration, $count ) = /^PULSE\d?\s+(\d+)\s*(\d*)/i ) ;

    $count = 1 if $count eq "" ;

    $sum += $duration * $count ;
}

print "PULSE $sum\n" ;

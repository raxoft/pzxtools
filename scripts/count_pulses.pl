#! /usr/bin/perl -w
#
# Create histogram of pulse durations encountered.
#
# Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
#
# This source code is released under the MIT license, see included license.txt.

my %stats ;

while(<>) {

    next unless ( my( $duration, $count ) = /^PULSE\d?\s+(\d+)\s*(\d*)/i ) ;

    $count = 1 if $count eq "" ;

    $stats{$duration} += $count ;
}

foreach my $duration ( sort { $a <=> $b } keys %stats ) {
    my $count = $stats{$duration} ;
    print "PULSE $duration $count\n" ;
}

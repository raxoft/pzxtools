#! /usr/bin/perl -wT
#
# Create histogram of pulse durations used.

my %stats ;

while(<>) {

    next unless ( my( $duration, $count ) = /^PULSE\s+(\d+)\s*(\d*)/i ) ;

    $count = 1 if $count eq "" ;

    $stats{$duration} += $count ;
}

foreach my $duration ( sort { $a <=> $b } keys %stats ) {
    my $count = $stats{$duration} ;
    print "PULSE $duration $count\n" ;
}
